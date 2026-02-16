#include "process.h"
#include "clist.h"
#include "kernel_config.h"
#include "scheduler.h"
#include "minilib/stddef.h"
#include "minilib/stdint.h"
#include "minilib/string.h"
#include "sync.h"
#include "time.h"
#include "tinyalloc.h"
#include "waitqueue.h"

#if TEST_CONFIG
#include "../tests/mocks/kernel_mocks.h"
#else
#include "cpu.h"
#endif

#define RA_INDEX 0
#define SP_INDEX 1
#define S0_INDEX 4

#define MAX_PROC 32

// Process table
typedef struct {
  uint8_t next_pid;
  uint8_t active_process; // Not TERMINATED
  clist_node_t head;
} ptable_t;

// Process table init
static ptable_t proc_table;

/**
 * @brief Init the process table.
 */
void init_proc_table() {
  clist_init_node(&proc_table.head);
  proc_table.next_pid = 0;
  proc_table.active_process = 0;
}

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
clist_node_t *get_ready_node(process_t *proc) { return &proc->ready_node; }
clist_node_t *get_wait_node(process_t *proc) { return &proc->wait_node; }
clist_node_t *get_sleep_node(process_t *proc) { return &proc->sleep_node; }
wait_queue_t *get_current_wq(process_t *proc) { return proc->current_wq; }

// Active process init
static process_t *active = NULL;

/* Active getters*/
process_t *get_active() { return active; }
uint8_t get_active_pid() { return active->pid; }
char *get_active_name() { return active->name; }

void switch_active(process_t *next) {
  if (active->state == RUNNING)
    active->state = READY;

  next->state = RUNNING;
  active = next;
}

static void _clear_from_blocking_queues(process_t *proc) {
  // Remove it if it from sleeping queue (timeout).
  if (is_in_sleeping_queue(proc)) {
    remove_from_sleeping(proc);
  };

  if (clist_is_in_list(&proc->wait_node)) {
    clist_remove(&proc->wait_node);
  }
}

/** STATE hanlding **/
/** Set a processus in sleeping state with a timer **/
void process_sleep(process_t *proc, uint32_t delay) {
  proc->state = SLEEPING;
  proc->wake_up_time = delay + seconds();
}

/** Set a processus to blocked state (waiting for IO irq) **/
void process_block(process_t *proc) { proc->state = BLOCKED; }

/** Switch the state of a sleeping process to running **/
void process_wake(process_t *proc) {
  if ((proc->state == SLEEPING) || (proc->state == BLOCKED)) {
    // Remove it from sleeping and blocked queue if remaining
    _clear_from_blocking_queues(proc);
    proc->state = RUNNING;
  }
}

/**
 * @brief Remove the process from all the queues he could be.
 *
 * @param proc Pointer to the processus to remove from queues.
 */
static inline void remove_from_all_queues(process_t *proc) {
  if (clist_is_in_list(&proc->proc_node))
    clist_remove(&proc->proc_node);
  if (clist_is_in_list(&proc->ready_node))
    clist_remove(&proc->ready_node);
  if (clist_is_in_list(&proc->sleep_node))
    clist_remove(&proc->sleep_node);
  if (clist_is_in_list(&proc->wait_node))
    clist_remove(&proc->wait_node);
}
/**
 * @brief Clean Up a process removing it from all queues and from memory.
 *
 * @param proc Pointer to the pocess to clean up.
 */
static inline void process_clean_up(process_t *proc) {
  proc->state = TERMINATED;
  remove_from_all_queues(proc);
  free(proc);
  proc_table.active_process--;
}

/**
 * @brief Zombify a process putting it in the zombies queue of the parent.
 *
 * @param proc Pointer to the processus to zombify.
 */
static inline void process_zombify(process_t *proc) {
  process_t *parent = proc->parent;
  if (!parent)
    return;

  proc->state = ZOMBIE;

  wq_enqueue(&parent->zombies, &proc->wait_node);
  if (parent->state == BLOCKED) {
    // Parent is waiting so we wake him up
    // to check if he can stop wait.
    scheduler_wake_waiting_queue(&parent->child_wq);
  }
}

/**
 * @brief Terminate a process.
 *
 * @note Handle parent and orphan.
 *
 * @param proc Pointer to the processus to terminate.
 */
void process_terminate(process_t *proc) {
  process_t *parent = proc->parent;
  if (parent) {
    process_zombify(proc);
  } else {
    process_clean_up(proc);
  }
}

/**
 * @brief Reap a zombie (Parent call this).
 *
 * @param proc Pointer to the processus to reap.
 */
void process_reap(process_t *proc) {
  if (proc->state != ZOMBIE) {
    return;
  }
  process_clean_up(proc);
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
  clist_init_node(&proc->proc_node);
  clist_init_node(&proc->ready_node);
  clist_init_node(&proc->wait_node);
  clist_init_node(&proc->sleep_node);
  wq_init(&proc->zombies);
  wq_init(&proc->child_wq);
}

/** Config a process in it slot in the process table  **/
static void config_process(void code(), char *nom, priority prior,
                           process_t *proc) {
  init_process_queues(proc);
  proc->pid = proc_table.next_pid;
  strncpy(proc->name, nom, MAXNAME - 1);
  proc->priority = prior;
  proc->state = READY;

  if (proc->pid == 0) {
    proc->ctx[RA_INDEX] = (uintptr_t)idle;
  } else {
    proc->ctx[SP_INDEX] = (uint64_t)&proc->stack[STACK_SIZE - 1];
    proc->ctx[RA_INDEX] = (uintptr_t)proc_launcher;
    proc->ctx[S0_INDEX] = (uintptr_t)code;
  }
}

/** Add a process to the running queue **/
static process_t *add_process_to_scheduler(process_t *slot) {
  proc_table.active_process++;
  proc_table.next_pid++;
  scheduler_admit(slot);
  return slot;
}

/** Spawn a process **/
process_t *spawn_process(void code(), char *nom, priority prior) {
  if (proc_table.active_process >= MAX_PROC) {
    return NULL; // Error already max processus launched
  }
  process_t *proc = malloc(sizeof(process_t));
  if (!proc) {
    return NULL;
  }
  config_process(code, nom, prior, proc);
  clist_push_back(&proc_table.head, &proc->proc_node);
  return add_process_to_scheduler(proc);
}

/** Spawn a child process in foreground and wait for it **/
int8_t spawn_foreground(void code(), char *name, priority prior) {
  process_t *parent = get_active();
  process_t *child = spawn_process(code, name, prior);
  if (!child) {
    return -1;
  }
  child->parent = parent;
  uint8_t pid = child->pid;
  wait_pid(child->pid);
  return pid;
};

/**
 * @brief Create the idle process and init active as idle.
 */
static void init_idle() {
  process_t *init_proc = spawn_process(idle, "idle", IDLE);
  active = init_proc;
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
