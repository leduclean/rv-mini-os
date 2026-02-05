#include "console.h"
#include "parser.h"
#include "process.h"
#include "programs.h"
#include <stdio.h>
#include <string.h>

static void builtin_ps() {
  // TODO when dynamic allocation.
}

/* Builtin shell Command to list all the prog launchable */
static void builtin_help() {
  printf("You can run the following programs: \n");
  for (int i = 0; prog_tab[i].name; i++) {
    printf("- %s \n", prog_tab[i].name);
  }
}

/* builtin */
static prog_t builtin_tab[] = {
    {"help", builtin_help},
    {"ps", builtin_ps},
    {NULL, NULL},
};

static char line_buffer[MAX_COLS];

/** Cmd match in the prog table */
static prog_t *match(shell_cmd_desc_t *cmd, prog_t tab[]) {
  prog_t *matched = NULL;
  char *cmd_id = cmd->argv[0];
  for (int i = 0; tab[i].name; i++) {
    if (strcmp(cmd_id, tab[i].name) == 0) {
      matched = &tab[i];
      break;
    }
  }
  return matched;
}

/** Handle the matched prog **/
static inline void handle_prog(prog_t *prog) {
  spawn_process(prog->fn, prog->name, NORMAL);
}

/** Handle the user shell command **/
static void cmd_handler(shell_cmd_desc_t *cmd) {
  if (cmd->argc == 0)
    return;

  prog_t *matched = match(cmd, builtin_tab);
  if (!matched)
    matched = match(cmd, prog_tab);

  if (!matched) {
    printf("Unknown command entered. Please refer to `help` cmd.\n");
    return;
  }

  handle_prog(matched);
}

/** Shell main process **/
void shell() {
  for (;;) {
    if (parser_read_line(line_buffer) != 0) {
      shell_cmd_desc_t cmd = parser_get_cmd(line_buffer);
      cmd_handler(&cmd);
    }
  }
}
