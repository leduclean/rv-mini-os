#include "parser.h"

#include <err.h>
#include <stdio.h>
#include <string.h>

#include "syscall.h"

ssize_t parser_scan_line(char *line, size_t max_size)
{
	size_t i = 0;

	if (!max_size)
		return -EINVAL;

	for (;;) {
		char c;
		read(STDIN, &c, 1);
		if (c == '\n' || c == '\r') {
			printf("\r\n");
			break;
		}

		if (c == '\b' || c == 127) { /* backspace */
			if (i > 0) {
				i--;
				printf("\b \b");
			}
			continue;
		}

		if (i < max_size - 1) {
			line[i++] = c;
			printf("%c", c);
		}
	}

	line[i] = '\0';
	return i;
}

int parse_line(char *line, char *argv[], int max_args)
{
	int argc = 0;
	char *token = strtok(line, " ");
	while (token && argc < max_args - 1) {
		argv[argc] = token;
		argc++;
		token = strtok(NULL, " ");
	}
	argv[argc] = NULL;
	return argc;
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
