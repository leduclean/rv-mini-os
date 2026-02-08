#include "kernel/process/process.h"
#include "arch/riscv/cpu.h"
#include "kernel/sched/circ_queue.h"
#include "kernel/sync/sync.h"
#include "kernel/time/time.h"
#include "lib/stddef.h"
#include "lib/stdint.h"
#include "lib/string.h"
#include <kernel/sched/scheduler.h>

#define RA_INDEX 0
#define SP_INDEX 1
#define S0_INDEX 4

// Helpers for the waiting queue

void wq_init(wait_queue_t *wq) {
  wq->head = NULL;
  wq->tail = NULL;
}

process_t *wq_peek_head(wait_queue_t *wq) { return wq->head; }

void wq_enqueue(process_t *proc, wait_queue_t *wq) {
  if (!wq->head) {
    wq->head = wq->tail = proc;
    set_next_wait(proc, NULL);
    set_prev_wait(proc, NULL);
  } else {
    set_next_wait(wq->tail, proc);
    set_prev_wait(proc, wq->tail);
    wq->tail = proc;
    set_next_wait(proc, NULL);
  }
}

/** Remove an element from a waint queue queue **/
void wq_remove(process_t *proc, wait_queue_t *wq) {
  process_t *prev = get_prev_wait(proc);
  process_t *next = get_next_wait(proc);
  if (prev) {
    set_next_wait(prev, next);
  } else {
    wq->head = next;
  }

  if (next) {
    set_prev_wait(next, prev);
  } else {
    wq->tail = prev;
  }

  if (is_in_sleeping_queue(proc))
    remove_from_sleeping(proc);

  // Remove it from current waiting queue
  set_next_wait(proc, NULL);
  set_prev_wait(proc, NULL);
}

/** Remove the last element of the waiting queue **/
process_t *wq_pop_head(wait_queue_t *wq) {
  process_t *head = wq->head;
  if (!head)
    return NULL;
  wq_remove(head, wq);
  return head;
}

/** Remove an item by pid, it returns the item if found else a NULL pointer **/
process_t *wq_remove_by_pid(wait_queue_t *wq, int8_t pid) {
  process_t *cur = wq->head;
  while (cur && get_pid(cur) != pid)
    cur = get_next_wait(cur);

  if (!cur)
    return NULL;

  wq_remove(cur, wq);
  return cur;
}

uint8_t wq_is_empty(wait_queue_t *wq) { return wq_peek_head(wq) == NULL; }

// Process definition
struct process {
  uint8_t pid;
  char name[MAXNAME];
  state state;
  uint64_t ctx[MAX_REG_SAVED];
  uint64_t stack[STACK_SIZE];

  // Time out and sleep time
  uint64_t wake_up_time;
  // waiting queue double linked list pointer
  // used to put the process in zombie/IO/mutex wait.
  process_t *wait_prev, *wait_next;
  wait_queue_t *current_wq; // Is set to NULL if not blocked

  process_t *sleep_prev, *sleep_next;

  // Zombie state handling with parent
  process_t *parent;
  wait_queue_t child_wq;
  wait_queue_t zombies;

  priority priority;
};

// Getters and setter on public fields
uint64_t *get_ctx(process_t *proc) { return proc->ctx; }
uint8_t get_pid(process_t *proc) { return proc->pid; };
const char *get_name(process_t *proc) { return proc->name; }
uint32_t get_wake_up(process_t *proc) { return proc->wake_up_time; }
priority get_priority(process_t *proc) { return proc->priority; }
wait_queue_t *get_wait_child_queue(process_t *proc) { return &proc->child_wq; };
wait_queue_t *get_zombies(process_t *proc) { return &proc->zombies; };
void set_state(process_t *proc, state state) { proc->state = state; }

/** Get next element in the a waiting queue **/
process_t *get_next_wait(process_t *proc) { return proc->wait_next; }
process_t *get_prev_wait(process_t *proc) { return proc->wait_prev; }
wait_queue_t *get_current_wq(process_t *proc) { return proc->current_wq; }
process_t *get_next_sleep(process_t *proc) { return proc->sleep_next; }
process_t *get_prev_sleep(process_t *proc) { return proc->sleep_prev; }

/** Set the next element in a wait queue **/
void set_next_wait(process_t *proc, process_t *next) {
  proc->wait_next = next;
  if (next)
    next->wait_prev = proc;
}

/** Set the prev element in a wait queue **/
void set_prev_wait(process_t *proc, process_t *prev) { proc->wait_prev = prev; }

void set_wq(process_t *proc, wait_queue_t *wq) { proc->current_wq = wq; }

/** Set the next element in the sleeping queue **/
void set_next_sleep(process_t *proc, process_t *next) {
  proc->sleep_next = next;
  if (next)
    next->sleep_prev = proc;
}
/** Set the prev element in the sleeping queue **/
void set_prev_sleep(process_t *proc, process_t *prev) {
  proc->sleep_prev = prev;
}

// Active process init
static process_t *active = NULL;

/* Active getters*/
uint8_t get_active_pid() { return active->pid; }

char *get_active_name() { return active->name; }

/** STATE hanlding **/
/** Set a processus in sleeping state with a timer **/
void process_sleep(uint32_t delay) {
  active->state = SLEEPING;
  active->wake_up_time = delay + seconds();
}

/** Set a processus to blocked state (waiting for IO irq) **/
void process_block() { active->state = BLOCKED; }

/** Switch the state of a sleeping process to running **/
void process_wake(process_t *proc) {
  if ((proc->state == SLEEPING) || (proc->state == BLOCKED)) {
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
  process_t *parent = active->parent;
  if (parent) {
    active->state = ZOMBIE;
    wq_enqueue(active, &parent->zombies);
    if (parent->state == BLOCKED) {
      // Parent is waiting so we wake him up
      // to check if he can stop wait.
      scheduler_wake_waiting_queue(&parent->child_wq);
    }
  } else {
    active->state = TERMINATED;
  }
  proc_table.active_process--;
}

/** Boolean condition helper to dermine if a process has higher priority.
 * We consider that priority are sorted decremental.
 * **/
uint8_t higher_priority(priority prior, priority other) {
  return prior < other;
}
/* Processus Table setter */

/** Config a process in it slot in the process table  **/
static void config_process(void code(), char *nom, priority prior,
                           process_t *slot) {
  slot->pid = proc_table.next_pid;
  strncpy(slot->name, nom, MAXNAME - 1);
  slot->priority = prior;
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
static process_t *add_process_to_scheduler(process_t *slot) {
  proc_table.active_process++;
  proc_table.next_pid++;
  scheduler_admit(slot);
  return slot;
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
process_t *spawn_process(void code(), char *nom, priority prior) {
  if (proc_table.active_process >= MAX_PROC) {
    return NULL; // Error already max processus launched
  }
  process_t *slot = find_slot();
  if (!slot)
    return NULL;
  config_process(code, nom, prior, slot);
  return add_process_to_scheduler(slot);
}

/** Spawn a child process in foreground and wait for it **/
uint8_t spawn_foreground(void code(), char *name, priority prior) {
  process_t *parent = get_active();
  process_t *child = spawn_process(code, name, prior);
  child->parent = parent;
  wait_pid(child->pid);
  return child->pid;
};

/* Init the processus table */
void init_proc_table() { memset(&proc_table, 0, sizeof(proc_table)); }

/* Create the idle processus */
void init_idle() {
  process_t *init_proc = spawn_process(idle, "idle", IDLE);
  active = &proc_table.table[init_proc->pid];
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
