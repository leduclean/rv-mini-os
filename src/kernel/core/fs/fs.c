#include <stddef.h>
#include <stdint.h>

#include <lib/err.h>
#include <lib/string.h>

#include <kernel/bcache.h>
#include <kernel/fs.h>
#include <kernel/inode.h>
#include <kernel/process.h>
#include <user/kinterface.h>

// Stub structure for now
struct fs_meta superblock = {
	.nb_blk = FS_NB_BLK,
	.blk_size = FS_BLK_SIZE,
	.nb_ibm = FS_NB_IBM,
	.nb_inode = FS_NB_INODE,

	.nb_dblk = FS_NB_DBLK,
	.nb_dbm = FS_NB_DBM,
};

uint32_t fs_get_bm_bits_per_blk(void)
{
	return superblock.blk_size * CHAR_BIT;
}

uint32_t fs_get_nb_dbm(void)
{
	return superblock.nb_dbm;
}

uint32_t fs_get_nb_ibm(void)
{
	return superblock.nb_ibm;
}

bool fs_blk_is_data_blk(int blkno)
{
	return FS_DATA_START <= blkno &&
	       blkno < FS_DATA_START + superblock.nb_dblk;
}

/**
 * @class dirent
 * @brief Directory entry descriptor.
 *
 */
struct dirent {
	char name[MAXNAME];
	uint32_t inum;
};

#define MAX_DIR_ENTRY (MAX_FILE_SIZE / sizeof(struct dirent))

/**
 * @brief Root directory inode.
 */
struct inode *root;
static inline int alloc_root(void)
{
	root = inode_create(INODE_DIRECTORY);
	if (IS_ERR(root)) {
		return PTR_ERR(root);
	}

	return 0;
}

int fs_init(void)
{
	int res;

	bcache_init();
	res = inode_init();
	if (res < 0) {
		return res;
	}
	res = alloc_root();
	if (res < 0) {
		return res;
	}

	return 0;
}

static int dir_add_entry(struct inode *dir, const char *name, uint32_t inum)
{
	int len = strlen(name) + 1;
	if (len > MAXNAME) {
		return -ENAMETOOLONG;
	}

	struct dirent entry;
	entry.inum = inum;
	strncpy(entry.name, name, len);

	int res = 0;
	res = inode_lock(dir);
	if (res < 0) {
		return res;
	}
	//TODO: For now it only grows, we should reuse unlinked entry
	res = inode_write(dir, false, (char *)&entry, dir->content.size,
			  sizeof(entry));
	if (res < 0) {
		goto out;
	}
out:
	inode_unlock(dir);
	return res;
}

static int dir_lookup(struct inode *dir, const char *name, struct inode **i)
{
	int res;
	res = inode_lock(dir);
	if (res < 0) {
		return res;
	}

	if (dir->content.type != INODE_DIRECTORY) {
		inode_unlock(dir);
		return -ENOTDIR;
	}

	struct dirent entry;
	for (size_t off = 0; off < dir->content.size; off += sizeof(entry)) {
		res = inode_read(dir, false, (char *)&entry, off,
				 sizeof(entry));
		if (res < 0) {
			goto out;
		}
		if (entry.inum == 0) {
			continue;
		}
		if (strncmp(entry.name, name, MAXNAME) == 0) {
			*i = inode_get(entry.inum);
			if (!*i) {
				res = -EIO;
				goto out;
			} else {
				res = 0;
				goto out;
			};
		}
	}
	// Node not found
	res = -ENOENT;
out:
	inode_unlock(dir);
	return res;
}

#define FS_PATH_DELIMITATOR "/"

struct inode *fs_inode_from_path(const char *path, unsigned long flags,
				 enum inode_type leaf_type)
{
	size_t len = strlen(path);
	if (len < 1) {
		return ERR_PTR(-EINVAL);
	}

	if (path[0] != '/') {
		return ERR_PTR(-EINVAL);
	}


	char kpath[FS_MAX_PATH];
	strcpy(kpath, path);

	char *token = strtok(kpath, FS_PATH_DELIMITATOR);
	struct inode *curr = inode_get(root->id);

	int res;
	while (token != NULL) {
		char *next_token = strtok(NULL, FS_PATH_DELIMITATOR);
		struct inode *next = NULL;

		res = dir_lookup(curr, token, &next);
		if (res == -ENOENT) {
			if (flags & O_CREAT) {
				enum inode_type type = next_token
							       ? INODE_DIRECTORY
							       : leaf_type;
				struct inode *created = inode_create(type);
				if (IS_ERR(created)) {
					res = PTR_ERR(created);
					goto err_release_curr;
				}
				res = dir_add_entry(curr, token, created->id);
				if (res < 0) {
					goto err_release_curr;
				}
				next = created;

			} else {
				goto err_release_curr;
			}
		} else if (res < 0) {
			goto err_release_curr;
		}

		inode_put(curr);
		token = next_token;
		curr = next;
	}
	return curr;

err_release_curr:
	inode_put(curr);
	return ERR_PTR(res);
}

int fs_mknod(const char *path, int dev_id)
{
	struct inode *i = fs_inode_from_path(path, O_CREAT, INODE_DEVICE);
	if (IS_ERR(i)) {
		return PTR_ERR(i);
	}
	int res = inode_lock(i);
	if (res < 0) {
		return res;
	}

	i->content.dev_id = dev_id;
	inode_mark_dirty(i);
	inode_unlock(i);
	inode_put(i);

	return 0;
}
