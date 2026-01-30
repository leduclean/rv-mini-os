#pragma once
#include <stdint.h>

#define MAXNAME 16
#define MAX_REG_SAVED 18
#define STACK_SIZE 4096
#define PROC_TABLE_SIZE 32

// Process State table
typedef enum { FREE = 0, RUNNING, READY, SLEEPING, TERMINATED } state;

// Process declaration
typedef struct {
  uint8_t pid;
  char name[MAXNAME];
  state state;
  uint64_t ctx[MAX_REG_SAVED];
  uint64_t stack[STACK_SIZE];
  uint64_t wake_up_time;
} process_t;

// Process table
typedef struct {
  uint8_t next_pid;
  uint8_t active_process; // Not TERMINATED
  process_t table[PROC_TABLE_SIZE];
} ptable_t;

// Run Queue
typedef struct {
  process_t *queue[PROC_TABLE_SIZE];
  uint8_t head; // Idx active
  uint8_t tail; // Idx for next insertion
  uint8_t size; // number of active element
} run_queue_t;

extern ptable_t proc_table;
extern process_t *active;
extern run_queue_t run_queue;

// Idle Process declaration
extern void idle();
extern void init_proc();
extern int8_t creer_processus(void code(), char *name);
extern uint8_t mon_pid();
extern char *mon_nom();
