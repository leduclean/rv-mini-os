#pragma once
#include <stdint.h>

#define MAXNAME 16
#define MAX_REG_SAVED 18
#define STACK_SIZE 4096
#define PROC_TABLE_SIZE 32

// Process State table
typedef enum { RUNNING, READY, SLEEPING, TERMINATED } state;

// Process declaration
typedef struct {
  int8_t pid;
  char name[MAXNAME];
  state state;
  uint64_t ctx[MAX_REG_SAVED];
  uint64_t stack[STACK_SIZE];
  uint64_t wake_up_time;
} process_t;

// Process table
typedef struct {
  uint8_t current_pid;
  process_t table[PROC_TABLE_SIZE];
} ptable_t;

extern ptable_t proc_table;
extern process_t *actif;

// Idle Process declaration
extern void idle();

extern void init_proc();
extern int8_t creer_processus(void code(), char *name);
extern void dors(uint64_t nbr_secondes);
extern void fin_processus();
