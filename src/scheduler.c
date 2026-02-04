#include "scheduler.h"
#include "circ_queue.h"
#include "process.h"
#include "time.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>

extern void ctx_sw(uintptr_t old_ctx, uintptr_t new_ctx);

/** Main rescheduling function called to change context between proc**/
static void do_ctx_switch(process_t *next) {
  uint64_t *old_ctx = get_ctx(get_active());
  switch_active(next);
  ctx_sw((uintptr_t)old_ctx, (uintptr_t)get_ctx(next));
}

/** Priority handling **/

/* Priority mapped running queue table */
static circ_queue_t run_queues[PRIORITY_COUNT];

/* Keep track the highest non empty ready priority */
static priority highest_ready_tracking;

void static init_run_queues() {
  memset(&run_queues, 0, sizeof(run_queues));
  // The initiate prioity should be IDLE
  highest_ready_tracking = IDLE;
}

/** Pick the highest priority non-empty queue **/
static inline circ_queue_t *pick_highest(void) {
  return &run_queues[highest_ready_tracking];
}

/** Refresh the highest_ready_tracking state if the queue is empty **/
void refresh_high_prio() {
  // Test the supposate highest prio
  if (!is_empty(&run_queues[highest_ready_tracking])) {
    return;
  }

  // scan the other queues (lower priority state are higher in priority index )
  for (int p = highest_ready_tracking + 1; p < PRIORITY_COUNT; p++) {
    if (!is_empty(&run_queues[p])) {
      // Update the new highest prio
      highest_ready_tracking = p;
      break;
    }
  }
}

/** Enqueue the process in the correct priority queuee **/
static void enqueue_process(process_t *proc) {
  priority prior_class = get_priority(proc);
  circ_queue_t *rq = &run_queues[prior_class];
  enqueue(rq, proc);
}

/** Dequeue the process in the correct priority queuee **/
static void dequeue_process(process_t *proc) {
  priority prior_class = get_priority(proc);
  circ_queue_t *rq = &run_queues[prior_class];
  dequeue(rq);
}

/** Check if premption is needed and do it if needed **/
static void check_and_preempt(process_t *proc) {
  priority prior = get_priority(proc);
  priority current_prior = get_priority(get_active());
  if (higher_priority(prior, current_prior)) {
    // Immediatly switch to this process
    do_ctx_switch(proc);
  }
}

/** Check if a process should update highest ready priority queue and do it **/
static void update_highest(process_t *proc) {
  priority prior_class = get_priority(proc);
  if (higher_priority(prior_class, highest_ready_tracking)) {
    highest_ready_tracking = prior_class;
  }
}

/** Admit a process with a level of priority in the corresponding queue **/
void scheduler_admit(process_t *proc) {
  enqueue_process(proc);
  update_highest(proc);
  check_and_preempt(proc);
}

/* Schedule function trigered by an interupt */
void scheduler_rotate() {
  circ_queue_t *current_runqueue = pick_highest();
  rotate_head_to_tail(current_runqueue);
  do_ctx_switch(peek_head(current_runqueue));
}

/* Schedule function trigered by an inactivity of the process
 * ie terminason, or sleep
 * */
static void switch_out_active() {
  refresh_high_prio();
  do_ctx_switch(peek_head(pick_highest()));
}

/** Terminate a processus **/
void scheduler_terminate() {
  dequeue_process(get_active());
  process_terminate();
  switch_out_active();
}

/* Launcher to handle launch and terminaison of a proc */
void proc_launcher(void proc()) {
  proc();
  scheduler_terminate();
}

/** Sleeping queue handling **/
static process_t *sleeping_head;

static void init_sleep_queue() { sleeping_head = NULL; }

static void insert_sleep(process_t *proc) {
  // If no sleeping head
  if (!sleeping_head || get_wake_up(proc) < get_wake_up(sleeping_head)) {
    set_next_sleeping(proc, sleeping_head);
    sleeping_head = proc;
    return;
  }
  process_t *prev = sleeping_head;
  process_t *current = get_next_sleeping(prev);

  while (current && get_wake_up(current) <= get_wake_up(proc)) {
    prev = current;
    current = get_next_sleeping(current);
  }

  set_next_sleeping(proc, current);
  set_next_sleeping(prev, proc);
}

/** Set a program to sleeping state **/
void scheduler_sleep(uint32_t nbr_secs) {
  process_t *proc = get_active();
  dequeue_process(proc);
  process_sleep(nbr_secs);
  insert_sleep(proc);
  switch_out_active();
}

/** Block on a specific queue relative to a peripheric **/
void scheduler_block_on(circ_queue_t *q) {
  process_t *proc = get_active();
  dequeue_process(proc);
  enqueue(q, proc);
  switch_out_active();
}

/** Wake up a process and reschedule it in the running queue **/
static void scheduler_ready_process(process_t *proc) {
  // Wake the process
  process_wake(proc);

  // Update the highest if needed
  update_highest(proc);

  // Insert in the correct run queue
  enqueue_process(proc);

  // Preempt if needed
  check_and_preempt(proc);
}

/** Wake up the process wakable in the sleeping queue **/
void scheduler_wake_sleeping() {
  uint32_t now = seconds();
  while (sleeping_head && get_wake_up(sleeping_head) <= now) {
    process_t *proc = sleeping_head;
    process_t *next = get_next_sleeping(proc);

    // detach first
    set_next_sleeping(proc, NULL);
    sleeping_head = next;

    scheduler_ready_process(proc);
  }
}

/** Wake all the process from a peripheric queue **/
void scheduler_wake_blocked_queue(circ_queue_t *q) {
  for (int i = 0; i < q->size; i++) {
    process_t *p = pop(q);
    scheduler_ready_process(p);
  }
}

void init_scheduler_queues() {
  init_run_queues();
  init_sleep_queue();
}
