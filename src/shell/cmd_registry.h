#pragma once
#include "kernel/process/process.h"
typedef enum { CMD_BUILTIN, CMD_PROG } exec_type;

typedef struct {
  void (*fn)();
  priority prior;
} prog_t;

typedef struct {
  char *name;
  exec_type type;
  union {
    void (*builtin)();
    prog_t prog;
  } cmd;
} cmd_desc_t;

void clear_cmd_registry();

int8_t register_builtin(char *name, void (*fn)());
int8_t register_prog(char *name, void (*fn)(), priority prior);

const cmd_desc_t *command_lookup(const char *name);

const cmd_desc_t *registry_get_nth(uint8_t idx);
uint8_t registry_get_size();
