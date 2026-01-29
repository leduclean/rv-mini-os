#pragma once
#include <stdint.h>

#define MAXNAME 16
#define MAX_REG_SAVED 18
#define STACK_SIZE 4096

// Process State table
typedef enum {
  ELECTED,
  ACTIVABLE,
} state;

// Process declaration
typedef struct {
  int8_t pid;
  char name[MAXNAME];
  state state;
  uint64_t ctx[MAX_REG_SAVED];
  uint64_t stack[STACK_SIZE];
} process_t;

// Process declaration
extern void idle();
extern void proc1();
extern void init_proc();
