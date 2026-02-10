#include "kernel/sched/scheduler.h"
#include "arch/riscv/cpu.h"
#include "kernel/process/process.h"
#include "kernel/sched/circ_queue.h"
#include "kernel/sync/waitqueue.h"
#include "kernel/time/time.h"
#include "lib/clist.h"
#include "lib/container.h"
#include "lib/stddef.h"
#include "lib/stdint.h"
#include "lib/string.h"

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

static void init_run_queues() {
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
  irq_flags_t flags = irq_save();

  enqueue_process(proc);
  update_highest(proc);
  check_and_preempt(proc);

  irq_restore(flags);
}

/* Schedule function trigered by an interupt */
void scheduler_rotate() {
  irq_flags_t flags = irq_save();

  circ_queue_t *current_runqueue = pick_highest();
  rotate_head_to_tail(current_runqueue);
  do_ctx_switch(peek_head(current_runqueue));

  irq_restore(flags);
}

/* Schedule function trigered by an inactivity of the process
 * ie terminason, or sleep
 * */
static void switch_out_active() {
  irq_flags_t flags = irq_save();

  refresh_high_prio();
  process_t *next = peek_head(pick_highest());
  do_ctx_switch(next);

  irq_restore(flags);
}

/** Terminate a processus **/
void scheduler_terminate() {
  irq_flags_t flags = irq_save();

  dequeue_process(get_active());
  process_terminate();
  switch_out_active();

  irq_restore(flags);
}

/* Launcher to handle launch and terminaison of a proc */
void proc_launcher(void proc()) {
  proc();
  scheduler_terminate();
}

/** Wake up a process and reschedule it in the running queue **/
void scheduler_ready_process(process_t *proc) {
  irq_flags_t flags = irq_save();

  // Wake the process
  process_wake(proc);

  // Update the highest if needed
  update_highest(proc);

  // Insert in the correct run queue
  enqueue_process(proc);

  // Preempt if needed
  check_and_preempt(proc);
  irq_restore(flags);
}

/** Sleeping queue handling **/
static clist_node_t sleeping_head;

static void init_sleep_queue() { clist_init_node(&sleeping_head); }

uint8_t is_in_sleeping_queue(process_t *proc) {
  return clist_is_in_list(get_sleep_node(proc));
}

static inline int _wake_up_cmp(clist_node_t *current, clist_node_t *other) {
  process_t *cur_proc = container_of(current, process_t, wait_node);
  process_t *other_proc = container_of(other, process_t, wait_node);
  return (get_wake_up(cur_proc) > get_wake_up(other_proc));
}

/** Insert a node in the clist respecting a wake up time
 * sort politic **/
static void insert_sleep(process_t *proc) {
  clist_node_t *node = get_sleep_node(proc);
  clist_insert_sorted(&sleeping_head, node, _wake_up_cmp);
}

/** Remove an element from the sleeping queue **/
void remove_from_sleeping(process_t *proc) {
  clist_remove(get_sleep_node(proc));
}

/** Set a program to sleeping state **/
void scheduler_sleep(uint32_t nbr_secs) {
  irq_flags_t flags = irq_save();

  process_t *proc = get_active();
  dequeue_process(proc);
  process_sleep(nbr_secs);
  insert_sleep(proc);
  switch_out_active();
  irq_restore(flags);
}

/**
 * @brief Wake up policy applied on each item of a sleeping queue.
 *
 * @param node Node of the process.
 * @param arg Now timer.
 * @return -1 to break the for each iteration on sorted list. 0 else.
 */
static inline int _wake_up_sleeping_cb(clist_node_t *node, void *arg) {
  uint32_t now = *(uint32_t *)arg;
  process_t *proc = container_of(node, process_t, sleep_node);
  if (get_wake_up(proc) > now)
    return -1;

  clist_remove(node);
  scheduler_ready_process(proc);
  return 0;
}

/** Wake up the wakable processes in the sleeping queue **/
void scheduler_wake_sleeping() {
  irq_flags_t flags = irq_save();

  uint32_t now = seconds();
  clist_for_each(&sleeping_head, _wake_up_sleeping_cb, &now);

  irq_restore(flags);
}

/** Block on a specific waiting queue relative to a signal **/
void scheduler_block_on(wait_queue_t *wq) {
  irq_flags_t flags = irq_save();

  process_t *proc = get_active();
  dequeue_process(proc);
  process_block();
  wq_enqueue(wq, get_wait_node(proc));
  set_wq(proc, wq);
  switch_out_active();

  irq_restore(flags);
}

/** Block on a specific waiting queue but with a timeout **/
void scheduler_block_on_with_timeout(wait_queue_t *wq, uint32_t timeout_secs) {
  irq_flags_t flags = irq_save();

  process_t *proc = get_active();
  dequeue_process(proc);
  process_block();
  wq_enqueue(wq, get_wait_node(proc));
  set_wq(proc, wq);

  if (timeout_secs > 0) {
    process_sleep(timeout_secs);
    insert_sleep(proc);
  }
  switch_out_active();

  irq_restore(flags);
}

/**
 * @brief Wake up policy applied to a wq.
 *
 * @param current Node to compute on.
 * @param args Aditional args.
 * @return 0 (invariant)
 */
static inline int _wake_up_waiting_cb(clist_node_t *current, void *args) {
  // Wake up the process from the waiting queue
  scheduler_ready_process(container_of(current, process_t, wait_node));
  return 0;
}

/**
 * @brief Wake an entire wait queue.
 *
 * This function is supposed to be called on an event to remove
 * process block on this event.
 *
 * @param wq Waiting queue associated with the block condition.
 */
void scheduler_wake_waiting_queue(wait_queue_t *wq) {
  irq_flags_t flags = irq_save();
  wq_for_each_and_del(wq, _wake_up_waiting_cb, NULL);
  irq_restore(flags);
}

void init_scheduler_queues() {
  init_run_queues();
  init_sleep_queue();
}
