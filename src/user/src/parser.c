#include "parser.h"

#include <string.h>

#include "syscall.h"

ssize_t parser_scan_line(char *line, size_t max_size)
{
	size_t i = 0;

	if (!max_size)
		return -1;

	for (;;) {
		char c;
		read(STDIN, &c, 1);
		if (c == '\n' || c == '\r') {
			write(STDOUT, "\r\n", sizeof("\r\n") - 1);
			break;
		}

		if (c == '\b' || c == 127) { /* backspace */
			if (i > 0) {
				i--;
				write(STDOUT, "\b \b", sizeof("\b \b") - 1);
			}
			continue;
		}

		if (i < max_size - 1) {
			line[i++] = c;
			write(STDOUT, &c, 1);
		}
	}

	line[i] = '\0';
	return i;
}

bool parser_is_bg(int *argc, char *argv[])
{
	if ((*argc > 0) && strcmp(argv[*argc - 1], "&") == 0) {
		(*argc)--;
		argv[*argc] = NULL;
		return true;
	}
	return false;
}
