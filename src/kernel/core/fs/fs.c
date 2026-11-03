#include <stddef.h>
#include <stdint.h>

#include <lib/err.h>
#include <lib/string.h>

#include <kernel/bcache.h>
#include <kernel/fs.h>
#include <kernel/fs_layout.h>
#include <kernel/inode.h>
#include <kernel/task.h>
#include <user/kinterface.h>

/**
 * @brief Structure filled at spawn reading the superblock.
 */
struct fs_meta sb;

#define MAX_DIR_ENTRY (MAX_FILE_SIZE / sizeof(struct dirent))

/**
 * @brief Root directory inode.
 */
struct inode *root;

static int set_root(void)
{
	root = inode_get(FS_ROOT_INODE_ID);
	inode_lock(root);
	if (root->content.type != INODE_DIRECTORY) {
		inode_unlock(root);
		return -EIO;
	}
	inode_unlock(root);
	return 0;
}

static inline int check_format(void)
{
	int res = 0;
	struct cblk *blk = bcache_read(FS_SUPERBLK);
	if (!blk) {
		res = -EIO;
		goto out;
	}

	memcpy((void *)&sb, blk->buf, sizeof(sb));

	if (sb.magic != FS_MAGIC_NUMBER) {
		res = -EIO;
		goto out;
	}

	if (sb.blk_size != FS_BLK_SIZE) {
		res = -EIO;
		goto out;
	}

out:
	bcache_release(blk);
	return res;
}

int fs_init(void)
{
	int res;
	bcache_init();
	res = check_format();
	if (res < 0) {
		return res;
	}
	res = inode_init();
	if (res < 0) {
		return res;
	}

	res = set_root();
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

/* @warning You obtain a reference of the inode that MUST be inode_put(). */
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
		inode_put(i);
		return res;
	}

	i->content.dev_id = dev_id;
	inode_mark_dirty(i);
	inode_unlock(i);
	inode_put(i);

	return 0;
}
