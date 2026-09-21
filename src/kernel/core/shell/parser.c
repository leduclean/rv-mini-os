#include <stddef.h>

#include <lib/stdio.h>
#include <lib/string.h>

#include <drivers/console.h>
#include <drivers/uart.h>

#include <kernel/parser.h>
#include <kernel/scheduler.h>

/**
 * @brief Buffer flush.
 *
 * @param buf Buffer to flush.
 * @param size Size of @p buf.
 */
static void _flush(char *buf, size_t size)
{
	memset(&buf, 0, size);
}

int parser_read_line(char *buf, size_t size)
{
	_flush(buf, size);
	int len = 0;
	for (;;) {
		char c;
		uart_read(&c);

		if ((c == '\n') || (c == '\r')) {
			buf[len] = '\0';
			printf("\n");
			return len;
		}

		if (c == '\b' || c == 127) { // backspace
			if (len > 0) {
				len--;
				printf("\b \b");
			}
			continue;
		}

		if (len < MAX_COLS - 1) {
			buf[len++] = c;
			// Echo the current character
			printf("%c", c);
		}
	}
}

/**
 * @brief If the last cmd argument is &, set background and drop that arg.
 *
 * @param cmd Command to check on.
 */
static void _check_background(shell_cmd_tokens_t *cmd)
{
	if ((cmd->argc > 0) && strcmp(cmd->argv[cmd->argc - 1], "&") == 0) {
		cmd->background = 1;
		cmd->argc--;
	}
}

shell_cmd_tokens_t parser_get_cmd(char *buf)
{
	shell_cmd_tokens_t cmd = { 0 };
	char *token = NULL;
	token = strtok(buf, " ");
	while (token && (cmd.argc < MAX_ARGS)) {
		cmd.argv[cmd.argc++] = token;
		token = strtok(NULL, " ");
	}

	_check_background(&cmd);
	return cmd;
}
