#include "cmd_registry.h"
#include "console.h"
#include "minilib/stddef.h"
#include "minilib/stdint.h"
#include "minilib/stdio.h"
#include "parser.h"
#include "process.h"
#include "programs.h"

/* Builtin shell Command to list all the prog launchable */
static void builtin_help() {
  printf("Available commands: \n");
  const cmd_desc_t *cmd = NULL;
  for (uint8_t i = 0; i < registry_get_size(); i++) {
    cmd = registry_get_nth(i);
    switch (cmd->type) {
    case CMD_BUILTIN:
      printf("- [builtin] %s\n", cmd->name);
      break;
    case CMD_PROG:
      printf("- [program] %s (priority: %d)\n", cmd->name, cmd->cmd.prog.prior);
      break;
    }
  }
}

static inline void init_builtins() { register_builtin("help", builtin_help); }

/** Handle the user shell command **/
static void cmd_handler(shell_cmd_tokens_t *cmd) {
  if (cmd->argc == 0)
    return;

  // Lookup the command in the registry
  const cmd_desc_t *matched = command_lookup(cmd->argv[0]);
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
    spawn_process(matched->cmd.prog.fn, matched->name,
                  matched->cmd.prog.prior); // background: no wait
  } else {
    spawn_foreground(
        matched->cmd.prog.fn, matched->name,
        matched->cmd.prog.prior); // foreground: blocks until child terminates
  }
}

static char line_buffer[MAX_COLS];

/** Shell main process **/
void shell() {
  init_builtins();
  init_programs();
  for (;;) {
    if (parser_read_line(line_buffer, MAX_COLS) != 0) {
      shell_cmd_tokens_t cmd = parser_get_cmd(line_buffer);
      cmd_handler(&cmd);
    }
  }
}
