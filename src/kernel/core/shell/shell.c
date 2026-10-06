#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lib/stdio.h>

#include <drivers/console.h>

#include <kernel/cmd_registry.h>
#include <kernel/parser.h>
#include <kernel/process.h>
#include <kernel/programs.h>

/**
 * @brief Handle a user shell command.
 *
 * @note A builtin is executed directly, a program is spawned in background
 * or in foreground depending on the command background flag.
 *
 * @param cmd Tokenized command to run.
 */
static void _cmd_handler(shell_cmd_tokens_t *cmd)
{
	if (cmd->argc == 0)
		return;

	// Lookup the command in the registry
	const cmd_desc_t *matched = cmd_lookup(cmd->argv[0]);
	if (!matched) {
		printf("Unknown command entered. Please refer to `help`.\n");
		return;
	}

	// Builtin: executed directly
	if (matched->type == CMD_BUILTIN) {
		matched->cmd.builtin();
		return;
	}

	// Program: spawn background or foreground
	if (cmd->background) {
		process_spawn(matched->cmd.prog.fn, matched->name,
			      matched->cmd.prog.prior,
			      matched->cmd.prog.user); // background: no wait
	} else {
		process_spawn_foreground(
			matched->cmd.prog.fn, matched->name,
			matched->cmd.prog.prior,
			matched->cmd.prog
				.user); // foreground: blocks until child terminates
	}
}

static char line_buffer[MAX_COLS];

void shell_run(void)
{
	programs_init();
	for (;;) {
		if (parser_read_line(line_buffer, MAX_COLS) != 0) {
			shell_cmd_tokens_t cmd = parser_get_cmd(line_buffer);
			_cmd_handler(&cmd);
		}
	}
}
