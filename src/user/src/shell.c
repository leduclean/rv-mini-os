#include "shell.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <kinterface.h>
#include <stdio.h>

#include "cmd_registry.h"
#include "parser.h"
#include "syscall.h"

int sh(void)
{
	int res;

	char line[MAX_DISPLAY_COLS];
	for (;;) {
		printf("(shell) ");
		if (parser_scan_line(line, MAX_DISPLAY_COLS) != 0) {
			struct cmd_desc cmd = { 0 };
			res = cmd_get_desc(line, &cmd);
			if (res < 0) {
				printf("Command not found, Please refer to help.\n");
				continue;
			}

			int8_t pid = fork();
			if (pid < 0) {
				res = pid;
				printf("Shell: Failed to fork \n");
				return res;
			} else if (res == 0) {
				printf("executing %s \n", cmd.argv[0]);
				res = exec(cmd.argv[0], cmd.argv);
				if (res < 0) {
					printf("Shell: Failed to exec \n");
					return res;
				}
			} else {
				if (!cmd.background) {
					res = waitpid(pid);
					if (res < 0) {
						return res;
					}
				}
			}
		}
	}
}
