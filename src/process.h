#pragma once
#include <stdint.h>

#define MAXNAME 16
#define MAX_REG_SAVED 18
#define STACK_SIZE 4096
#define MAX_PROC 32

// Process State table
typedef enum { FREE = 0, RUNNING, READY, SLEEPING, TERMINATED } state;

// Process definition
typedef struct process process_t;

extern process_t *get_active();
extern uint64_t *get_ctx(process_t *proc);
extern uint32_t get_wake_up(process_t *proc);
extern uint8_t get_active_pid();
extern char *get_active_name();
extern void switch_active(process_t *next);
extern void process_sleep(uint32_t delay); // delay is in secondes
extern void process_wake(process_t *proc);
extern void process_terminate();

// Idle Process declaration
extern void idle();
extern void init_proc();
extern int8_t spawn_process(void code(), char *name);
