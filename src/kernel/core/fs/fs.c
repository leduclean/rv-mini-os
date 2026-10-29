#include <stddef.h>
#include <stdint.h>

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
	if (!root) {
		return -1;
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

void fs_put(struct file *f)
{
	f->refcnt--;
}

void fs_get(struct file *f)
{
	f->refcnt++;
}

#define FS_FILE 100 /**< Max number of open files. */

static struct file ftable[FS_FILE] = { 0 };

struct file *file_alloc(void)
{
	for (int i = 0; i < FS_FILE; i++) {
		if (ftable[i].refcnt == 0) {
			return &ftable[i];
		}
	}

	return NULL;
}

struct dev_info {
	write_cb w_op;
	read_cb r_op;
};

#define FS_MAX_DEV 30

static struct dev_info dev_map[FS_MAX_DEV];

void fs_map_dev_cb(int dev_id, write_cb w_op, read_cb r_op)
{
	dev_map[dev_id].r_op = r_op;
	dev_map[dev_id].w_op = w_op;
}

static int dir_add_entry(struct inode *dir, const char *name, uint32_t inum)
{
	int len = strlen(name) + 1;
	if (len > MAXNAME) {
		return -1;
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

#define ENOENT 2 /**< Path error when a requested entry doesn't exists */

static int dir_lookup(struct inode *dir, const char *name, struct inode **i)
{
	int res;
	res = inode_lock(dir);
	if (res < 0) {
		return res;
	}

	if (dir->content.type != INODE_DIRECTORY) {
		return -1;
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
				res = -1;
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

#define PATH_DELIMITATOR "/"

static struct inode *inode_from_path(char *path, unsigned long flags)
{
	size_t len = strlen(path);
	if (len < 1) {
		return NULL;
	}
	if (path[0] != '/') {
		return NULL;
	}

	char *token = strtok(path, PATH_DELIMITATOR);
	struct inode *curr = inode_get(root->id);

	int res;
	while (token != NULL) {
		char *next_token = strtok(NULL, PATH_DELIMITATOR);
		struct inode *next = NULL;

		res = dir_lookup(curr, token, &next);
		if (res == -ENOENT) {
			if (flags & O_CREAT) {
				enum inode_type type = next_token
							       ? INODE_DIRECTORY
							       : INODE_FILE;
				struct inode *created = inode_create(type);
				if (created == NULL) {
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
	return NULL;
}

static ssize_t file_write(struct file *f, const char *ubuf, size_t count)
{
	int res;
	ssize_t written;

	struct inode *i = inode_get(f->inum);
	if (!i) {
		res = -1;
		goto err;
	}
	res = inode_lock(i);
	if (res < 0) {
		goto err_put_inode;
	}

	written = inode_write(i, true, ubuf, f->off, count);
	if (written < 0) {
		goto err_release_inode;
	}
	res = inode_mark_dirty(i);
	if (res < 0) {
		goto err_release_inode;
	}

	f->off += written;
	res = written;

err_release_inode:
	inode_unlock(i);
err_put_inode:
	inode_put(i);
err:
	return res;
}

static ssize_t file_read(struct file *f, char *ubuf, size_t count)
{
	int res;
	ssize_t readed;
	struct inode *i = inode_get(f->inum);
	if (!i) {
		res = -1;
		goto err;
	}
	res = inode_lock(i);
	if (res < 0) {
		goto err_put_inode;
	}

	readed = inode_read(i, true, ubuf, f->off, count);
	if (readed < 0) {
		goto err_release_inode;
	}

	f->off += readed;
	res = readed;
err_release_inode:
	inode_unlock(i);
err_put_inode:
	inode_put(i);
err:
	return res;
}

static void link_dev_cb(struct file *f, int dev_id)
{
	f->r_op = dev_map[dev_id].r_op;
	f->w_op = dev_map[dev_id].w_op;
}

struct file *fs_open(char *path, unsigned long flags)
{
	int res;
	struct inode *i = inode_from_path(path, flags);
	if (!i) {
		return NULL;
	}

	struct file *f = file_alloc();
	if (!f) {
		goto err_release_inode;
	}

	res = inode_lock(i);
	if (res < 0) {
		goto err_release_inode;
	}

	if (i->content.type == INODE_DEVICE) {
		link_dev_cb(f, i->content.dev_id);
	} else {
		f->r_op = file_read;
		f->w_op = file_write;
	}

	inode_unlock(i);

	f->inum = i->id;
	f->mode = flags & O_MODE_MASK;
	f->off = 0;
	f->refcnt = 1;

	return f;

err_release_inode:
	inode_put(i);
	return NULL;
}
