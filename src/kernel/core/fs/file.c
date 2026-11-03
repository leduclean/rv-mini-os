#include <stddef.h>
#include <stdint.h>

#include <lib/err.h>
#include <lib/string.h>

#include <kernel/file.h>
#include <kernel/inode.h>
#include <kernel/task.h>
#include <user/kinterface.h>

#define FILE_MAX_NUM 100 /**< Max number of open files. */
static struct file ftable[FILE_MAX_NUM] = { 0 };

static struct file *file_alloc(void)
{
	for (int i = 0; i < FILE_MAX_NUM; i++) {
		if (ftable[i].refcnt == 0) {
			return &ftable[i];
		}
	}

	return NULL;
}

struct file *file_clone(const struct file *f)
{
	struct file *new = file_alloc();
	if (!new) {
		return NULL;
	}
	*new = *f;
	/* Add a reference to the inode */
	new->i = inode_get(f->i->id);
	new->refcnt = 1;

	return new;
}

void file_put(struct file *f)
{
	if (f->refcnt-- == 1) {
		inode_put(f->i);
	}
}

struct file *file_get(struct file *f)
{
	f->refcnt++;
	return f;
}

struct dev_info {
	write_cb w_op;
	read_cb r_op;
};

#define FILE_MAX_DEV 30
/**
 * @brief Device to write/read callback mapping.
 */
static struct dev_info dev_map[FILE_MAX_DEV];

void file_map_dev_cb(int dev_id, write_cb w_op, read_cb r_op)
{
	dev_map[dev_id].r_op = r_op;
	dev_map[dev_id].w_op = w_op;
}

static ssize_t file_write(struct file *f, const char *ubuf, size_t count)
{
	int res;
	ssize_t written;
	struct inode *i = f->i;

	res = inode_lock(i);
	if (res < 0) {
		return -EIO;
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
	return res;
}

static ssize_t file_read(struct file *f, char *ubuf, size_t count)
{
	int res;
	ssize_t readed;
	struct inode *i = f->i;

	res = inode_lock(i);
	if (res < 0) {
		return -EIO;
	}

	readed = inode_read(i, true, ubuf, f->off, count);
	if (readed < 0) {
		goto err_release_inode;
	}

	f->off += readed;
	res = readed;
err_release_inode:
	inode_unlock(i);
	return res;
}

static void link_dev_cb(struct file *f, int dev_id)
{
	f->r_op = dev_map[dev_id].r_op;
	f->w_op = dev_map[dev_id].w_op;
}

struct file *file_open(const char *path, unsigned long flags)
{
	int res;
	struct inode *i = fs_inode_from_path(path, flags, INODE_FILE);
	if (IS_ERR(i)) {
		return ERR_PTR(PTR_ERR(i));
	}

	struct file *f = file_alloc();
	if (!f) {
		res = -ENFILE;
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

	f->i = i;
	f->mode = flags & O_MODE_MASK;
	f->off = 0;
	f->refcnt = 1;

	return f;

err_release_inode:
	inode_put(i);
	return ERR_PTR(res);
}

int64_t file_seek(struct file *f, int64_t offset, int whence)
{
	int64_t next;
	switch (whence) {
	case SEEK_SET:
		next = offset;
		break;
	case SEEK_CUR:
		next = f->off + offset;
		break;
	case SEEK_END:
		struct inode *i = f->i;
		int res = inode_lock(i);
		if (res < 0) {
			return res;
		}
		next = i->content.size + offset;
		inode_unlock(i);
		break;
	default:
		return -EINVAL;
	}

	if (next < 0) {
		return -EINVAL;
	}

	f->off = next;
	return next;
}
