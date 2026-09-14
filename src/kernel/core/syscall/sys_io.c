#include "console.h"
#include "cpu.h"
#include "process.h"
#include "syscall.h"
#include "minilib/stddef.h"
#include "minilib/types.h"
#include "vpages.h"

#define CONSOLE_FD 1 /* Console file descriptor number */

ssize_t sys_write(int fd, const char *ubuf, size_t count)
{
	if (count >= MAX_COLS) {
		return -1;
	}

	char kbuf[MAX_COLS];
	pte_t *root = process_active()->root_ptable;

	// The ubuf is something pointing to a virtual address of the process.
	// To gain access to it in the kernel we need to copy in.
	int res = vpage_copyin(root, kbuf, ubuf, count);
	if (res < 0) {
		return res;
	}

	if (fd == CONSOLE_FD) {
		console_putbytes(kbuf, count);
	} else {
		panic("We do not handle other file descriptor than the screen for now");
	}

	return count;
}
