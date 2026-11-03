#define _POSIX_C_SOURCE 200809L /* pread/pwrite under -std=c23 */

#include <stdint.h>

#include <fcntl.h>
#include <kernel/fs_layout.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#ifndef DISK_SIZE_M
#error "Must provide a disk size"
#else
#define DISK_SIZE (DISK_SIZE_M << 20)
#endif
/**
 * @name Metadata Superblock values.
 * @{
 */
#define FS_NB_BLK (DISK_SIZE / FS_BLK_SIZE) /**< Number of blk in the fs. */
#define FS_NB_RESERVED 1 /**< Number of reserved blocks. */
#define FS_NB_SUPERBLK 1 /**< Number of superblock blocks. */
#define FS_NB_IBM 2 /**< Number of inode bit map blocks. */
#define FS_NB_DBM 2 /**< Number of data bit map blocks. */
#define FS_NB_ITAB 5 /**< Number of inode tab blocks*/
#define FS_NB_DBLK                                                         \
	(FS_NB_BLK - FS_NB_ITAB - FS_NB_IBM - FS_NB_DBM - FS_NB_SUPERBLK - \
	 FS_NB_RESERVED) /**< Number of data blocks. */
/** @} */

/**
 * @brief Tracks the number of inode allocated to set theire
 * associated index in the bit map.
 */
static int i_allocated = 0;
/**
 * @brief Tracks the number of blk allocated to set theire
 * associated index in the bit map.
 */
static int blk_allocated = 0;

/**
 * @brief Super block metadata of the filesystem.
 */
const struct fs_meta superblock = {
	.magic = FS_MAGIC_NUMBER,
	.nb_blk = FS_NB_BLK,
	.blk_size = FS_BLK_SIZE,
	.nb_ibm = FS_NB_IBM,
	.nb_dbm = FS_NB_DBM,
	.nb_itab = FS_NB_ITAB,
	.nb_dblk = FS_NB_DBLK,
	.ibm_start = FS_SUPERBLK + FS_NB_SUPERBLK,
	.dbm_start = FS_SUPERBLK + FS_NB_SUPERBLK + FS_NB_IBM,
	.itab_start = FS_SUPERBLK + FS_NB_SUPERBLK + FS_NB_IBM + FS_NB_DBM,
	.data_start = FS_SUPERBLK + FS_NB_SUPERBLK + FS_NB_IBM + FS_NB_DBM +
		      FS_NB_ITAB
};

/**
 * @brief Root directory of the file system.
 */
static struct dinode root_di = {
	.type = INODE_DIRECTORY,
	.nblink = 1,
	.dirblkno = { superblock.data_start },
};

/** Inodes the itab can hold. */
#define FS_NB_INODES (FS_NB_ITAB * FS_BLK_SIZE / sizeof(struct dinode))

/* Allocation is sequential on a fresh disk: the bitmaps are just the first
 * N bits set, written once at the end by write_bm(). */
static int alloc_block(void)
{
	if (++blk_allocated >= FS_BM_BITS_PER_BLK ||
	    blk_allocated >= FS_NB_DBLK) {
		fprintf(stderr,
			"Mkfs should use only the first block bit map\n");
		return -1;
	}

	return blk_allocated + superblock.data_start;
}

static int write_inode(int disk_fd, int ino, const struct dinode *i)
{
	off_t off = (off_t)superblock.itab_start * FS_BLK_SIZE +
		    ino * sizeof(struct dinode);
	if (pwrite(disk_fd, i, sizeof(*i), off) < 0) {
		perror("Could not write the inode tab in the disk");
		return -1;
	}

	return 0;
}

static int alloc_inode(int disk_fd, const struct dinode *i)
{
	if (++i_allocated >= FS_BM_BITS_PER_BLK ||
	    i_allocated >= (int)FS_NB_INODES) {
		fprintf(stderr,
			"Mkfs should use only the first inode bit map\n");
		return -1;
	}

	if (write_inode(disk_fd, i_allocated, i) < 0)
		return -1;

	return i_allocated;
}

/**
 * @brief Set bits [0, @p nbits) of the bitmap block @p start.
 * @return 0 on success, -1 on error.
 */
static int write_bm(int disk_fd, uint32_t start, int nbits)
{
	uint8_t bm[FS_BLK_SIZE] = {};

	memset(bm, 0xff, nbits / CHAR_BIT);
	if (nbits % CHAR_BIT)
		bm[nbits / CHAR_BIT] = (1 << (nbits % CHAR_BIT)) - 1;

	if (pwrite(disk_fd, bm, sizeof(bm), (off_t)start * FS_BLK_SIZE) < 0) {
		perror("Could not write a bitmap in the disk");
		return -1;
	}

	return 0;
}

static inline off_t data_off(int dblkno)
{
	return ((off_t)dblkno) * FS_BLK_SIZE;
}

/**
 * @brief Stream the host file @p name into freshly allocated data blocks.
 * @param i Filled with size/type/dirblkno; the caller writes it to the itab.
 */
static int alloc_file(int disk_fd, const char *name, struct dinode *i)
{
	int fd = open(name, O_RDONLY);
	if (fd < 0) {
		perror(name);
		return -1;
	}
	struct stat stat;
	if (fstat(fd, &stat) < 0) {
		perror("Failed to get the file stat");
		goto err;
	}

	if (stat.st_size > FS_MAX_FILE_SIZE) {
		fprintf(stderr, "%s: file too big %jd, should be at max: %zu\n",
			name, stat.st_size, FS_MAX_FILE_SIZE);
		goto err;
	}

	*i = (struct dinode){ .type = INODE_FILE, .nblink = 1 };

	char buf[FS_BLK_SIZE];
	ssize_t n;
	int dir_blk = 0;
	int nb_indir = 0;
	while ((n = read(fd, buf, sizeof(buf))) > 0) {
		int blkno = alloc_block();
		if (blkno < 0)
			goto err;

		if (pwrite(disk_fd, buf, n, data_off(blkno)) != n) {
			perror("Could not write file data in the disk");
			goto err;
		}

		if (dir_blk == FS_INODE_DIR_DBLKNO) {
			if (!i->indir) {
				int indir_blk = alloc_block();
				if (indir_blk < 0) {
					goto err;
				}
				i->indir = indir_blk;
			}

			if (pwrite(disk_fd, &blkno, sizeof(blkno),
				   data_off(i->indir) +
					   nb_indir * sizeof(blkno)) !=
			    sizeof(blkno)) {
				perror("Could not write file data in the disk");
				goto err;
			}

			nb_indir++;
		} else {
			i->dirblkno[dir_blk++] = blkno;
		}
		i->size += n;
	}
	if (n < 0) {
		perror(name);
		goto err;
	}

	close(fd);
	return 0;
err:
	close(fd);
	return -1;
}

/**
 * @brief Append (basename of @p path, @p id) to @p dir's first data block.
 * @return 0 on success, -1 on error.
 */
static int dir_add_entry(int disk_fd, struct dinode *dir, const char *path,
			 uint32_t id)
{
	const char *slash = strrchr(path, '/');
	const char *name = slash ? slash + 1 : path;

	if (strlen(name) >= MAXNAME) {
		fprintf(stderr, "%s: name longer than %d chars\n", name,
			MAXNAME - 1);
		return -1;
	}

	if (dir->size + sizeof(struct dirent) > FS_BLK_SIZE) {
		fprintf(stderr,
			"Mkfs should only use a blk size for builtin entries\n");
		return -1;
	}

	struct dirent entry = { .inum = id };
	strcpy(entry.name, name);

	off_t off = data_off(dir->dirblkno[0]) + dir->size;
	if (pwrite(disk_fd, &entry, sizeof(entry), off) < 0) {
		perror("Could not write the dir entry in the disk");
		return -1;
	}
	dir->size += sizeof(entry);

	return 0;
}

int main(int argc, char *argv[])
{
	off_t off;
	int res;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <disk>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int fd = open(argv[1], O_RDWR | O_CREAT | O_TRUNC, 0644);
	if (fd < 0) {
		perror("Could not open the disk");
		return EXIT_FAILURE;
	}

	res = ftruncate(fd, FS_NB_BLK * FS_BLK_SIZE);
	if (res < 0) {
		perror("Could not truncate the fd of the disk");
		return EXIT_FAILURE;
	}

	off = FS_SUPERBLK * FS_BLK_SIZE;
	if (pwrite(fd, &superblock, sizeof(superblock), off) < 0) {
		perror("Could not write the superblock in the disk");
		return EXIT_FAILURE;
	}

	/* Reserve the root inode now, rewrite it once its size is known. */
	res = alloc_inode(fd, &root_di);
	if (res < 0) {
		return EXIT_FAILURE;
	}

	for (int i = 2; i < argc; i++) {
		struct dinode file_i;
		res = alloc_file(fd, argv[i], &file_i);
		if (res < 0) {
			return EXIT_FAILURE;
		}

		res = alloc_inode(fd, &file_i);
		if (res < 0) {
			return EXIT_FAILURE;
		}
		res = dir_add_entry(fd, &root_di, argv[i], res);
		if (res < 0) {
			return EXIT_FAILURE;
		}
	}

	if (write_inode(fd, FS_ROOT_INODE_ID, &root_di) < 0)
		return EXIT_FAILURE;

	/* +1: bit 0 is the reserved inode / the root data blk 0. */
	if (write_bm(fd, superblock.ibm_start, i_allocated + 1) < 0 ||
	    write_bm(fd, superblock.dbm_start, blk_allocated + 1) < 0)
		return EXIT_FAILURE;

	close(fd);
	return EXIT_SUCCESS;
}
