#pragma once

#include <stddef.h>
#define MAX_ARGS 4

typedef struct {
  int argc;
  char *argv[MAX_ARGS];
  int background; // 0 or 1 for &
} shell_cmd_tokens_t;

int parser_read_line(char *buf, size_t size);
shell_cmd_tokens_t parser_get_cmd(char *buf);
