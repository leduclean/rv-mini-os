#pragma once
#include "kernel/sched/circ_queue.h"
#include "kernel/sync/waitqueue.h"
#include "lib/stdint.h"

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
uint64_t *get_ctx(process_t *proc);
uint32_t get_wake_up(process_t *proc);
priority get_priority(process_t *proc);
const char *get_name(process_t *proc);
uint8_t get_pid(process_t *proc);
process_t *get_next_wait(process_t *proc);
process_t *get_prev_wait(process_t *proc);
process_t *get_next_sleep(process_t *proc);
process_t *get_prev_sleep(process_t *proc);

wait_queue_t *get_current_wq(process_t *proc);
wait_queue_t *get_zombies(process_t *proc);
wait_queue_t *get_wait_child_queue(process_t *proc);

// Process setter
void set_state(process_t *proc, state state);
void set_next_wait(process_t *proc, process_t *next);
void set_prev_wait(process_t *proc, process_t *prev);
void set_wq(process_t *proc, wait_queue_t *wq);
void set_next_sleep(process_t *proc, process_t *next);
void set_prev_sleep(process_t *proc, process_t *prev);
void switch_active(process_t *next);

// Active getter
process_t *get_active();
uint8_t get_active_pid();
char *get_active_name();

uint8_t higher_priority(priority prior, priority other);

void process_sleep(uint32_t delay); // delay is in secondes
void process_block();
void process_wake(process_t *proc);
void process_terminate();

// Idle Process declaration
void idle();
void init_proc();
process_t *spawn_process(void code(), char *name, priority prior);
uint8_t spawn_foreground(void code(), char *name, priority prior);
