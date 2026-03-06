#include "scheduler.h"
#include "clist.h"
#include "container.h"
#include "cpu.h"
#include "minilib/stddef.h"
#include "minilib/string.h"
#include "process.h"
#include "time.h"
#include "waitqueue.h"

extern void ctx_sw(uintptr_t old_ctx, uintptr_t new_ctx);

/** Main rescheduling function called to change context between proc**/
static void do_ctx_switch(process_t *next) {
  uint64_t *old_ctx = get_ctx(get_active());
  switch_active(next);
  ctx_sw((uintptr_t)old_ctx, (uintptr_t)get_ctx(next));
}

/** Priority handling **/

/**
 * @brief Priority mapped ready queues.
 */
static clist_node_t ready_queues[PRIORITY_COUNT];
/**
 * @brief Priority flag to keep track of the highest priority.
 */
static priority highest_ready_tracking;

/**
 * @brief Init all the ready queues and the priority.
 */
static void init_ready_queues() {
  // Reset all the node of the ready queue
  for (int i = 0; i < PRIORITY_COUNT; i++) {
    clist_node_t *current_head = &ready_queues[i];
    clist_init_node(current_head);
  }
  // The initiate prioity should be IDLE
  highest_ready_tracking = IDLE;
}

/**
 * @brief Add a process to the end of its corresponding priority ready queue.
 *
 * @param proc Pointer to the processus we want to add.
 */
static void ready_queue_enqueue(process_t *proc) {
  priority prior_class = get_priority(proc);
  clist_node_t *rq = &ready_queues[prior_class];
  clist_push_back(rq, &proc->ready_node);
}

/**
 * @brief Remove the process from the ready queue he is in.
 *
 * @param proc The processus to remove.
 */
static inline void ready_queue_remove(process_t *proc) {
  if (!clist_is_in_list(&proc->ready_node))
    return;
  clist_remove(&proc->ready_node);
}

/**
 * @brief Round Robin circular rotation of the ready queue and giv
 *
 * @note This function also gives the next processus to switch on.
 *
 * @param rq Pointer to the ready queue to rotate on.
 * @return Pointer to the next process.
 */
static process_t *rotate_ready_queue(clist_node_t *rq) {
  // Nothing to do if the queue is empty or there is only one element
  if (clist_empty(rq) || rq->next->next == rq)
    return container_of(clist_first(rq), process_t, ready_node);
  clist_node_t *first = clist_pop_front(rq);
  clist_push_back(rq, first);

  return container_of(clist_first(rq), process_t, ready_node);
}

/*
 * @brief Check if a priority preemption is needed and does it if needed.
 *
 * @param proc Pointer to the processus we want to add.
 */
static void check_and_preempt(process_t *proc) {
  priority prior = get_priority(proc);
  priority current_prior = get_priority(get_active());
  if (higher_priority(prior, current_prior)) {
    // Immediatly switch to this process
    do_ctx_switch(proc);
  }
}

/**
 * @brief Pick the highest priority ready queue in the ready queues
 *
 * @return Pointer to the highest priority queue (clist).
 */
static inline clist_node_t *pick_highest(void) {
  return &ready_queues[highest_ready_tracking];
}

/**
 * @brief Refresh highest priority flag scnaning all the queues.
 *
 * @note The scan is down increamenting the index since the highest priority is
 * 0.
 */
void refresh_highest_prio() {
  // Test the supposate highest prio
  if (!clist_empty(&ready_queues[highest_ready_tracking])) {
    return;
  }

  // scan the other queues (lower priority state are higher in priority index )
  for (int p = highest_ready_tracking + 1; p < PRIORITY_COUNT; p++) {
    if (!clist_empty(&ready_queues[p])) {
      // Update the new highest prio
      highest_ready_tracking = p;
      break;
    }
  }
}

/**
 * @brief Check if a process should change the highest priority flag.
 *
 * @param proc Pointer to the processus we want to check on.
 */
static void update_highest(process_t *proc) {
  priority prior_class = get_priority(proc);
  if (higher_priority(prior_class, highest_ready_tracking)) {
    highest_ready_tracking = prior_class;
  }
}

/**
 * @brief Admit a processus in the coresponding ready queue.
 *
 * @param proc Pointer to the processus to admit.
 */
void scheduler_admit(process_t *proc) {
  irq_flags_t flags = irq_save();

  ready_queue_enqueue(proc);
  update_highest(proc);
  check_and_preempt(proc);

  irq_restore(flags);
}

/* Schedule function trigered by an interupt */
void scheduler_rotate() {
  irq_flags_t flags = irq_save();

  process_t *next = rotate_ready_queue(pick_highest());
  if (next != get_active())
    do_ctx_switch(next);

  irq_restore(flags);
}

/* Schedule function trigered by an inactivity of the process
 * ie terminason, or sleep
 * */
static void switch_out_active() {
  irq_flags_t flags = irq_save();

  refresh_highest_prio();
  clist_node_t *next_node = clist_first(pick_highest());
  do_ctx_switch(container_of(next_node, process_t, ready_node));

  irq_restore(flags);
}

/** Terminate a processus **/
void scheduler_terminate() {
  irq_flags_t flags = irq_save();
  process_t *current = get_active();

  ready_queue_remove(current);
  process_terminate(current);
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
  ready_queue_enqueue(proc);

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
  ready_queue_remove(proc);
  process_sleep(proc, nbr_secs);
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

static inline void move_to_wq(process_t *proc, wait_queue_t *wq) {
  ready_queue_remove(proc);
  process_block(proc);
  wq_enqueue(wq, &proc->wait_node);
}

/** Block on a specific waiting queue relative to a signal **/
void scheduler_block_on(wait_queue_t *wq) {
  irq_flags_t flags = irq_save();

  process_t *proc = get_active();
  move_to_wq(proc, wq);
  switch_out_active();

  irq_restore(flags);
}

/** Block on a specific waiting queue but with a timeout **/
void scheduler_block_on_with_timeout(wait_queue_t *wq, uint32_t timeout_secs) {
  irq_flags_t flags = irq_save();

  process_t *proc = get_active();
  move_to_wq(proc, wq);
  if (timeout_secs > 0) {
    process_sleep(proc, timeout_secs);
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
  // args is unused but needed for the for a for each callback
  (void)args;
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
  init_ready_queues();
  init_sleep_queue();
}
