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
			char *argv[EXEC_MAX_ARGS];
			int argc = parse_line(line, argv, EXEC_MAX_ARGS);
			bool bg = parser_is_bg(&argc, argv);
			if (argc == 0) {
				printf("Please enter a command.\n");
				continue;
			}

			cmd_func_t f = cmd_find(argv[0]);
			if (!f) {
				printf("Command not found, Please refer to help.\n");
				continue;
			}

			int8_t pid = fork();
			if (pid < 0) {
				res = pid;
				printf("Shell: Failed to fork \n");
				exit_group(1);
			} else if (pid == 0) {
				res = exec(argv[0], argv);
				if (res < 0) {
					printf("Shell: Failed to exec \n");
					exit_group(1);
				}
			} else {
				if (!bg) {
					res = waitpid(pid);
					if (res < 0) {
						exit_group(1);
					}
				}
			}
		}
	}
}
