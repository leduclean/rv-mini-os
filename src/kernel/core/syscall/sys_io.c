#include <stddef.h>

#include <lib/string.h>
#include <lib/types.h>

#include <asm/cpu.h>

#include <drivers/console.h>
#include <drivers/uart.h>

#include <kernel/fs.h>
#include <kernel/inode.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>
#include <kernel/vpages.h>
#include <user/kinterface.h>

#define SYS_BUFFER_SIZE (1 << 10)
#define SYS_MAX_PATH \
	(10 *        \
	 (MAXNAME +  \
	  2)) /** Max size for a path, correspond to 10 Name + separator */

int sys_open(char *path, unsigned long flags)
{
	char kernel_path[SYS_MAX_PATH];

	process_t *active = process_active();
	int res = vpage_copyinstr(active->root_ptable, kernel_path, path,
				  SYS_MAX_PATH);
	if (res < 0) {
		return res;
	}

	struct file *f = fs_open(kernel_path, flags);
	if (!f) {
		return -1;
	}

	int fd = process_give_fd(active, f);
	if (fd < 0) {
		fs_put(f);
	}

	return fd;
}

ssize_t sys_write(int fd, const char *ubuf, size_t count)
{
	int res;
	ssize_t written;
	//TODO: STDOUT should be hardcoded int the process allocation
	// with inode data mapped to the console device ?
	if (fd == STDOUT) {
		if (count > SYS_BUFFER_SIZE) {
			return -1;
		}
		char kbuf[SYS_BUFFER_SIZE];
		pte_t *root = process_active()->root_ptable;
		// The ubuf is something pointing to a virtual address of the process.
		// To gain access to it in the kernel we need to copy in.
		res = vpage_copyin(root, kbuf, ubuf, count);
		if (res < 0) {
			return res;
		}
		console_putbytes(kbuf, count);
		return count;
	}

	process_t *active = process_active();
	struct file *f = process_get_fd(active, fd);
	if (!(f->mode & O_W)) {
		res = -1;
		goto err;
	}

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

ssize_t sys_read(int fd, char *ubuf, size_t count)
{
	int res;
	ssize_t readed;
	//TODO: STDIN should be hardcoded int the process allocation
	// with inode data mapped to the console device ?
	if (fd == STDIN) {
		if (count > SYS_BUFFER_SIZE) {
			return -1;
		}

		char kbuf[SYS_BUFFER_SIZE];
		pte_t *root = process_active()->root_ptable;

		for (size_t i = 0; i < count; i++) {
			uart_read(&kbuf[i]);
		}

		// The ubuf is something pointing to a virtual address of the process.
		// We need to copy to the physical address of the ubuf.
		res = vpage_copyout(root, ubuf, kbuf, count);
		if (res < 0) {
			return res;
		}
		return count;
	}

	process_t *active = process_active();
	struct file *f = process_get_fd(active, fd);
	if (!(f->mode & O_R)) {
		res = -1;
		goto err;
	}

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
