#include <stddef.h>

#include <lib/types.h>

#include <asm/cpu.h>

#include <drivers/console.h>
#include <drivers/uart.h>

#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>
#include <kernel/vpages.h>

#define CONSOLE_FD 1 /* Console file descriptor number */
#define SYS_BUFFER_SIZE (1 << 10)

ssize_t sys_write(int fd, const char *ubuf, size_t count)
{
	if (fd != CONSOLE_FD) {
		panic("We do not handle other file descriptor than the screen for now");
	}

	if (count > SYS_BUFFER_SIZE) {
		return -1;
	}

	char kbuf[SYS_BUFFER_SIZE];
	pte_t *root = process_active()->root_ptable;

	// The ubuf is something pointing to a virtual address of the process.
	// To gain access to it in the kernel we need to copy in.
	int res = vpage_copyin(root, kbuf, ubuf, count);
	if (res < 0) {
		return res;
	}

	console_putbytes(kbuf, count);

	return count;
}

ssize_t sys_read(int fd, char *ubuf, size_t count)
{
	if (fd != CONSOLE_FD) {
		panic("We do not handle other file descriptor than the screen for now");
	}

	if (count > SYS_BUFFER_SIZE) {
		return -1;
	}

	char kbuf[SYS_BUFFER_SIZE];
	pte_t *root = process_active()->root_ptable;
	ssize_t readed = 0;

	for (size_t i = 0; i < count; i++) {
		uart_read(&kbuf[i]);
		readed++;
	}

	// The ubuf is something pointing to a virtual address of the process.
	// We need to copy to the physical address of the ubuf.
	int res = vpage_copyout(root, ubuf, kbuf, count);
	if (res < 0) {
		return res;
	}

	return readed;
}
