#include "parser.h"
#include "console.h"
#include "minilib/stddef.h"
#include "minilib/stdio.h"
#include "minilib/string.h"
#include "scheduler.h"
#include "uart.h"

/* Buffer flush */
static void flush(char *buf, size_t size) { memset(&buf, 0, sizeof(size)); }

/** Processus that echo the uart character and write in the buffer
 * the current line **/
int parser_read_line(char *buf, size_t size) {
  flush(buf, size);
  int len = 0;
  for (;;) {
    char c;
    while (uart_read(&c) == -1) {
      scheduler_block_on(uart_get_wait_queue());
    }

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

/** If last cmd argument is & then set backround to true and remove the last arg
 * **/
static void check_background(shell_cmd_tokens_t *cmd) {
  if ((cmd->argc > 0) && strcmp(cmd->argv[cmd->argc - 1], "&") == 0) {
    cmd->background = 1;
    cmd->argc--;
  }
}

/** Tokenize the line and convert it into shell command **/
shell_cmd_tokens_t parser_get_cmd(char *buf) {
  shell_cmd_tokens_t cmd = {0};
  char *token = NULL;
  token = strtok(buf, " ");
  while (token && (cmd.argc < MAX_ARGS)) {
    cmd.argv[cmd.argc++] = token;
    token = strtok(NULL, " ");
  }

  check_background(&cmd);
  return cmd;
}
