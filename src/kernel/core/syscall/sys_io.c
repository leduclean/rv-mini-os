#include <stddef.h>
#include <stdint.h>

#include <lib/string.h>
#include <user/kinterface.h>

#include <asm/cpu.h>

#include <drivers/console.h>
#include <drivers/uart.h>

#include <kernel/file.h>
#include <kernel/inode.h>
#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>
#include <kernel/vpages.h>
#include <user/kinterface.h>

int sys_open(const char *path, unsigned long flags)
{
	char kernel_path[FS_MAX_PATH];

	process_t *active = process_active();
	int res = vpage_copyinstr(active->root_ptable, kernel_path, path,
				  FS_MAX_PATH);
	if (res < 0) {
		return res;
	}

	struct file *f = file_open(kernel_path, flags);
	if (!f) {
		return -1;
	}

	int fd = process_give_fd(active, f);
	if (fd < 0) {
		file_put(f);
	}

	return fd;
}

ssize_t sys_write(int fd, const char *ubuf, size_t count)
{
	process_t *active = process_active();
	struct file *f = process_get_fd(active, fd);
	if (!f) {
		return -1;
	}

	if (!(f->mode & O_W)) {
		return -1;
	}
	return f->w_op(f, ubuf, count);
}

ssize_t sys_read(int fd, char *ubuf, size_t count)
{
	process_t *active = process_active();
	struct file *f = process_get_fd(active, fd);
	if (!f) {
		return -1;
	}

	if (!(f->mode & O_R)) {
		return -1;
	}
	return f->r_op(f, ubuf, count);
}

int sys_close(int fd)
{
	process_t *active = process_active();
	return process_release_fd(active, fd);
}

int64_t sys_seek(int fd, int64_t offset, int whence)
{
	process_t *active = process_active();
	struct file *f = process_get_fd(active, fd);
	if (!f) {
		return -1;
	}

	return file_seek(f, offset, whence);
}
