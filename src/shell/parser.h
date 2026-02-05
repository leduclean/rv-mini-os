#pragma once

#define MAX_ARGS 4

typedef struct {
  int argc;
  char *argv[MAX_ARGS];
  int background; // 0 or 1 for &
} shell_cmd_desc_t;

int parser_read_line(char *buf);
shell_cmd_desc_t parser_get_cmd(char *buf);
