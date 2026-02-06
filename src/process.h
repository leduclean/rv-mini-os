#pragma once
#include "circ_queue.h"
#include "sync.h"
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

typedef struct wait_queue {
  process_t *head;
  process_t *tail;
} wait_queue_t;

// Helpers for the waiting queue
process_t *wq_peek_head(wait_queue_t *wq);

void wq_enqueue(process_t *proc, wait_queue_t *wq);
process_t *wq_dequeue(wait_queue_t *wq);
uint8_t wq_is_empty(wait_queue_t *wq);
process_t *wq_remove_by_pid(wait_queue_t *wq, int8_t pid);

// Process definition
typedef struct process process_t;

// Process getter
extern process_t *get_active();
extern uint64_t *get_ctx(process_t *proc);
extern uint32_t get_wake_up(process_t *proc);
extern priority get_priority(process_t *proc);
wait_queue_t *get_zombies(process_t *proc);
wait_queue_t *get_wait_child_queue(process_t *proc);
extern uint8_t get_active_pid();
extern char *get_active_name();
char *get_name(process_t *proc);
uint8_t get_pid(process_t *proc);
extern process_t *get_next_waiting(process_t *proc);

// Process setter
void set_state(process_t *proc, state state);
extern void set_next_wait(process_t *proc, process_t *next);
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
