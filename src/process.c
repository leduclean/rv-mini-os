#include "process.h"
#include "cpu.h"
#include "time.h"
#include <scheduler.h>
#include <stdint.h>
#include <string.h>

#define RA_INDEX 0
#define SP_INDEX 1
#define S0_INDEX 4

// Process definition
struct process {
  uint8_t pid;
  char name[MAXNAME];
  state state;
  uint64_t ctx[MAX_REG_SAVED];
  uint64_t stack[STACK_SIZE];
  uint64_t wake_up_time;
  // sleeping chained list pointer
  process_t *next_sleeping;
};

// Active process init
static process_t *active = NULL;

// Getters and setter on public fields
uint64_t *get_ctx(process_t *proc) { return proc->ctx; }
void set_state(process_t *proc, state state) { proc->state = state; }
uint32_t get_wake_up(process_t *proc) { return proc->wake_up_time; }

/** Set a processus in sleeping state with a timer **/
void process_sleep(uint32_t delay) {
  active->state = SLEEPING;
  active->wake_up_time = delay + seconds();
}

/** Get next element in the sleeping queue **/
process_t *get_next_sleeping(process_t *proc) { return proc->next_sleeping; }

/** Set the next element in the sleeping **/
void set_next_sleeping(process_t *proc, process_t *next) {
  proc->next_sleeping = next;
}
/** Switch the state of a sleeping process to running **/
void process_wake(process_t *proc) {
  if (proc->state == SLEEPING) {
    proc->state = RUNNING;
  }
}

// Process table
typedef struct {
  uint8_t next_pid;
  uint8_t active_process; // Not TERMINATED
  process_t table[MAX_PROC];
} ptable_t;

// Process table init
static ptable_t proc_table;

process_t *get_active() { return active; }

/** Switch the active process to a given process **/
void switch_active(process_t *next) {
  if (active->state == RUNNING)
    active->state = READY;

  next->state = RUNNING;
  active = next;
}

/** Switch to terminated state **/
void process_terminate() {
  active->state = TERMINATED;
  proc_table.active_process--;
}

/* Processus Table setter */

/** Config a process in it slot in the process table  **/
static void config_process(void code(), char *nom, process_t *slot) {
  slot->pid = proc_table.next_pid;
  strncpy(slot->name, nom, MAXNAME - 1);
  slot->state = READY;

  if (slot == &proc_table.table[0]) {
    slot->ctx[RA_INDEX] = (uintptr_t)idle;
  } else {
    slot->ctx[SP_INDEX] = (uint64_t)&slot->stack[STACK_SIZE - 1];
    slot->ctx[RA_INDEX] = (uintptr_t)proc_launcher;
    slot->ctx[S0_INDEX] = (uintptr_t)code;
  }
}

/** Add a process to the running queue **/
static int8_t add_process_to_scheduler(process_t *slot) {
  proc_table.active_process++;
  proc_table.next_pid++;
  enqueue(slot);
  return slot->pid;
}

/** Find an empty slot for a processus **/
static process_t *find_slot() {
  for (uint8_t slot_idx = 0; slot_idx < MAX_PROC; slot_idx++) {
    process_t *candidate = &proc_table.table[slot_idx];
    if (candidate->state == FREE || candidate->state == TERMINATED) {
      return candidate;
    }
  }
  return NULL;
}

/** Spawn a process **/
int8_t spawn_process(void code(), char *nom) {
  if (proc_table.active_process >= MAX_PROC) {
    return -1; // Error already max processus launched
  }
  process_t *slot = find_slot();
  if (!slot)
    return -1;
  config_process(code, nom, slot);
  return add_process_to_scheduler(slot);
}

/* Return active pid */
uint8_t get_active_pid() { return active->pid; }

/* Return the active name */
char *get_active_name() { return active->name; }

/* Init the processus table */
void init_proc_table() { memset(&proc_table, 0, sizeof(proc_table)); }

/* Create the idle processus */
void init_idle() {
  uint8_t init_pid = spawn_process(idle, "idle");
  active = &proc_table.table[init_pid];
  active->state = RUNNING;
}

/* Init processus handling */
void init_proc() {
  init_proc_table();
  init_scheduler_queues();
  init_idle();
}

/** IDLE Processus declaration  **/
void idle() {
  for (;;) {
    enable_it();
    hlt();
    disable_it();
  }
}
