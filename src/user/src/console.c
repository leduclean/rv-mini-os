#include "kinterface.h"
#include "syscall.h"

void console_putbytes(const char *s, int len)
{
	for (int i = 0; i < len; i++) {
		ssize_t written = write(STDOUT, &s[i], 1);
		if (written != 1) {
			exit(1);
		}
	};
}
