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
