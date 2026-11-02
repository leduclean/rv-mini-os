#define _POSIX_C_SOURCE 200809L /* pread/pwrite under -std=c23 */

#include <stdint.h>

#include <fcntl.h>
#include <kernel/fs_layout.h>
#include <stdio.h>
#include <stdlib.h>
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
 * @name Inode Bitmap set at mkfs bits.
 * @{
 */
#define FS_INODE_BM_RESERVED (1 << 0) /**< Reserved Inode bit. */
#define FS_INODE_BM_ROOT (1 << 1) /**< Root Inode bit. */
/** @} */

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
const struct dinode root_di = {
	.type = INODE_DIRECTORY,
	.nblink = 1,
};

int main(int argc, char *argv[])
{
	off_t off;
	uint8_t buf;
	int res;

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <disk>\n", argv[0]);
		return EXIT_FAILURE;
	}

	int fd = open(argv[1], O_RDWR | O_SYNC);
	if (fd < 0) {
		perror("Could not open the disk");
		return EXIT_FAILURE;
	}

	/* Reset to 0 the file */
	res = ftruncate(fd, 0);
	if (res < 0) {
		perror("Could not truncate the fd of the disk");
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

	off = superblock.ibm_start * FS_BLK_SIZE;
	if (pread(fd, &buf, sizeof(buf), off) < 0) {
		perror("Could not read the inode bitmap in the disk");
		return EXIT_FAILURE;
	}
	buf |= FS_INODE_BM_RESERVED | FS_INODE_BM_ROOT;
	if (pwrite(fd, &buf, sizeof(buf), off) < 0) {
		perror("Could not write the inode bitmap in the disk");
		return EXIT_FAILURE;
	}

	off = superblock.itab_start * FS_BLK_SIZE;
	if (pwrite(fd, &root_di, sizeof(root_di),
		   off + FS_ROOT_INODE_ID * sizeof(struct dinode)) < 0) {
		perror("Could not write the root in the inode tab in the disk");
		return EXIT_FAILURE;
	}

	close(fd);
	return EXIT_SUCCESS;
}
