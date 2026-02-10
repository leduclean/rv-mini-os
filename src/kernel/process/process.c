#include "kernel/process/process.h"
#include "arch/riscv/cpu.h"
#include "kernel/sched/circ_queue.h"
#include "kernel/sched/scheduler.h"
#include "kernel/sync/sync.h"
#include "kernel/sync/waitqueue.h"
#include "kernel/time/time.h"
#include "lib/clist.h"
#include "lib/stddef.h"
#include "lib/stdint.h"
#include "lib/string.h"

#define RA_INDEX 0
#define SP_INDEX 1
#define S0_INDEX 4

// Getters and setter on fields
uint64_t *get_ctx(process_t *proc) { return proc->ctx; }
uint8_t get_pid(process_t *proc) { return proc->pid; };
const char *get_name(process_t *proc) { return proc->name; }
uint32_t get_wake_up(process_t *proc) { return proc->wake_up_time; }
priority get_priority(process_t *proc) { return proc->priority; }
wait_queue_t *get_wait_child_queue(process_t *proc) { return &proc->child_wq; };
wait_queue_t *get_zombies(process_t *proc) { return &proc->zombies; };
void set_state(process_t *proc, state state) { proc->state = state; }

/** Get node **/
clist_node_t *get_wait_node(process_t *proc) { return &proc->wait_node; }
clist_node_t *get_sleep_node(process_t *proc) { return &proc->sleep_node; }
wait_queue_t *get_current_wq(process_t *proc) { return proc->current_wq; }

void set_wq(process_t *proc, wait_queue_t *wq) { proc->current_wq = wq; }

// Active process init
static process_t *active = NULL;

/* Active getters*/
uint8_t get_active_pid() { return active->pid; }

char *get_active_name() { return active->name; }

static void _clear_from_blocking_queues(process_t *proc) {
  // Remove it if it from sleeping queue (timeout).
  if (is_in_sleeping_queue(proc)) {
    remove_from_sleeping(proc);
  };

  clist_node_t *wait_node = get_wait_node(proc);
  if (clist_is_in_list(wait_node)) {
    clist_remove(wait_node);
  }
}

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
    // Remove it from sleeping and blocked queue if remaining
    _clear_from_blocking_queues(proc);
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
    wq_enqueue(&parent->zombies, &active->wait_node);
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

/** Reset all the node and the wq relative to a process **/
static inline void init_process_queues(process_t *proc) {
  clist_init_node(&proc->wait_node);
  clist_init_node(&proc->sleep_node);
  wq_init(&proc->zombies);
  wq_init(&proc->child_wq);
}
/** Config a process in it slot in the process table  **/
static void config_process(void code(), char *nom, priority prior,
                           process_t *slot) {
  init_process_queues(slot);
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
    hlt();
  }
}
