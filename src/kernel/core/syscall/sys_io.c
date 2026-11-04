#include <stddef.h>
#include <stdint.h>

#include <lib/err.h>
#include <lib/string.h>

#include <drivers/console.h>
#include <drivers/uart.h>

#include <arch/cpu.h>
#include <kernel/file.h>
#include <kernel/inode.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>
#include <kernel/task.h>
#include <kernel/vpages.h>
#include <user/kinterface.h>

int sys_open(const char *path, unsigned long flags)
{
	char kernel_path[FS_MAX_PATH];

	task_t *active = task_active();
	int res = vpage_copyinstr(active->root_ptable, kernel_path, path,
				  FS_MAX_PATH);
	if (res < 0) {
		return res;
	}

	struct file *f = file_open(kernel_path, flags);
	if (IS_ERR(f)) {
		return PTR_ERR(f);
	}

	int fd = task_give_fd(active, f);
	if (fd < 0) {
		file_put(f);
	}

	return fd;
}

ssize_t sys_write(int fd, const char *ubuf, size_t count)
{
	task_t *active = task_active();
	struct file *f = task_get_fd(active, fd);
	if (!f) {
		return -EBADF;
	}

	return file_write(f, true, ubuf, count);
}

ssize_t sys_read(int fd, char *ubuf, size_t count)
{
	task_t *active = task_active();
	struct file *f = task_get_fd(active, fd);
	if (!f) {
		return -EBADF;
	}

	return file_read(f, true, ubuf, count);
}

int sys_close(int fd)
{
	task_t *active = task_active();
	return task_release_fd(active, fd);
}

int64_t sys_seek(int fd, int64_t offset, int whence)
{
	task_t *active = task_active();
	struct file *f = task_get_fd(active, fd);
	if (!f) {
		return -EBADF;
	}

	return file_seek(f, offset, whence);
}
