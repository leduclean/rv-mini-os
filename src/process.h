#pragma once
#include "circ_queue.h"
#include <stdint.h>

#define MAXNAME 16
#define MAX_REG_SAVED 18
#define STACK_SIZE 4096

// Process State table
typedef enum {
  FREE = 0,
  RUNNING,
  READY,
  SLEEPING,
  BLOCKED,
  TERMINATED,
  ZOMBIE
} state;

typedef enum { HIGH = 0, NORMAL, LOW, IDLE, PRIORITY_COUNT } priority;

// Process definition
typedef struct process process_t;

// Process getter
extern process_t *get_active();
extern uint64_t *get_ctx(process_t *proc);
extern uint32_t get_wake_up(process_t *proc);
extern priority get_priority(process_t *proc);
circ_queue_t *get_zombies(process_t *proc);
circ_queue_t *get_wait_child_queue(process_t *proc);
extern uint8_t get_active_pid();
extern char *get_active_name();
uint8_t get_pid(process_t *proc);
extern process_t *get_next_sleeping(process_t *proc);

// Process setter
void set_state(process_t *proc, state state);
extern void set_next_sleeping(process_t *proc, process_t *next);
extern void switch_active(process_t *next);

extern uint8_t higher_priority(priority prior, priority other);

extern void process_sleep(uint32_t delay); // delay is in secondes
void process_block();
extern void process_wake(process_t *proc);
extern void process_terminate();

// Idle Process declaration
extern void idle();
extern void init_proc();
extern process_t *spawn_process(void code(), char *name, priority prior);
uint8_t spawn_foreground(void code(), char *name, priority prior);
