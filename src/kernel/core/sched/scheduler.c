#include "scheduler.h"
#include "clist.h"
#include "container.h"
#include "cpu.h"
#include "minilib/stddef.h"
#include "minilib/stdio.h"
#include "mmap.h"
#include "process.h"
#include "time.h"
#include "waitqueue.h"

extern void ctx_sw(ctx_t *old_ctx, ctx_t *new_ctx);

/**
 * @brief Main rescheduling function, changing the context between processes.
 *
 * @param next Process to switch on.
 */
static void _do_ctx_switch(process_t *next)
{
	ctx_t *old_ctx = get_ctx(get_active());
	switch_active(next);
	ctx_sw(old_ctx, get_ctx(next));
}

// Priority handling

/** @brief Priority mapped ready queues. */
static clist_node_t ready_queues[PRIORITY_COUNT];

/** @brief Highest ready priority. */
static priority highest_ready_tracking;

/** @brief Init all the ready queues and the priority flag. */
static void _init_ready_queues()
{
	// Reset all the node of the ready queue
	for (int i = 0; i < PRIORITY_COUNT; i++) {
		clist_node_t *current_head = &ready_queues[i];
		clist_init_node(current_head);
	}
	// The initiate prioity should be IDLE
	highest_ready_tracking = IDLE;
}

/**
 * @brief Add a process at the end of its corresponding priority ready queue.
 *
 * @param proc Process to enqueue.
 */
static void _ready_queue_enqueue(process_t *proc)
{
	priority prior_class = get_priority(proc);
	clist_node_t *rq = &ready_queues[prior_class];
	clist_push_back(rq, &proc->ready_node);
}

/**
 * @brief Remove a process from the ready queue it is in.
 *
 * @param proc Process to remove.
 */
static inline void _ready_queue_remove(process_t *proc)
{
	if (!clist_is_in_list(&proc->ready_node))
		return;
	clist_remove(&proc->ready_node);
}

/**
 * @brief Round robin circular rotation of a ready queue.
 *
 * @note This function also gives the next process to switch on.
 *
 * @param rq Ready queue to rotate on.
 * @return Pointer to the next process.
 */
static process_t *_rotate_ready_queue(clist_node_t *rq)
{
	// Nothing to do if the queue is empty or there is only one element
	if (clist_empty(rq) || rq->next->next == rq)
		return container_of(clist_first(rq), process_t, ready_node);
	clist_node_t *first = clist_pop_front(rq);
	clist_push_back(rq, first);

	return container_of(clist_first(rq), process_t, ready_node);
}

/**
 * @brief Check if a priority preemption is needed and does it if needed.
 *
 * @param proc Process that may preempt the active one.
 */
static void _check_and_preempt(process_t *proc)
{
	process_t *current = get_active();

	if (!current) {
		return;
	}

	priority prior = get_priority(proc);
	priority current_prior = get_priority(current);
	if (higher_priority(prior, current_prior)) {
		// Immediatly switch to this process
		_do_ctx_switch(proc);
	}
}

/**
 * @brief Pick the highest priority ready queue in the ready queues.
 *
 * @return Pointer to the highest priority queue (clist).
 */
static inline clist_node_t *_pick_highest(void)
{
	return &ready_queues[highest_ready_tracking];
}

/**
 * @brief Refresh the highest priority flag, scanning all the queues.
 *
 * @note The scan is done incrementing the index, since the highest priority
 * is 0.
 */
void refresh_highest_prio()
{
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
 * @param proc Process to check on.
 */
static void _update_highest(process_t *proc)
{
	priority prior_class = get_priority(proc);
	if (higher_priority(prior_class, highest_ready_tracking)) {
		highest_ready_tracking = prior_class;
	}
}

void scheduler_admit(process_t *proc)
{
	irq_flags_t flags = irq_save();

	_ready_queue_enqueue(proc);
	_update_highest(proc);
	_check_and_preempt(proc);

	irq_restore(flags);
}

void scheduler_rotate()
{
	irq_flags_t flags = irq_save();

	process_t *next = _rotate_ready_queue(_pick_highest());
	if (next != get_active())
		_do_ctx_switch(next);

	irq_restore(flags);
}

/**
 * @brief Switch out the active process on its inactivity.
 *
 * @note Called on a terminaison or on a sleep, it switches on the head of
 * the highest priority ready queue.
 */
static void _switch_out_active()
{
	irq_flags_t flags = irq_save();

	refresh_highest_prio();
	clist_node_t *next_node = clist_first(_pick_highest());
	_do_ctx_switch(container_of(next_node, process_t, ready_node));

	irq_restore(flags);
}

void scheduler_terminate(int exit_code)
{
	irq_flags_t flags = irq_save();
	process_t *current = get_active();

	_ready_queue_remove(current);
	process_terminate(current, exit_code);
	_switch_out_active();

	irq_restore(flags);
}

void proc_launcher()
{
	process_t *p = get_active();
	if (p->user) {
		enter_user_mode(p);
	} else {
		p->code();
		scheduler_terminate(0);
	}
}

void scheduler_ready_process(process_t *proc)
{
	irq_flags_t flags = irq_save();

	// Wake the process
	process_wake(proc);

	// Update the highest if needed
	_update_highest(proc);

	// Insert in the correct run queue
	_ready_queue_enqueue(proc);

	// Preempt if needed
	_check_and_preempt(proc);
	irq_restore(flags);
}

// Sleeping queue handling
static clist_node_t sleeping_head;

/** @brief Init the sleeping queue. */
static void _init_sleep_queue()
{
	clist_init_node(&sleeping_head);
}

uint8_t is_in_sleeping_queue(process_t *proc)
{
	return clist_is_in_list(get_sleep_node(proc));
}

/**
 * @brief Sort policy of the sleeping queue.
 *
 * @param current Node already in the queue.
 * @param other Node being inserted.
 * @return 1 if @p current wakes up after @p other, 0 otherwise.
 */
static inline int _wake_up_cmp(clist_node_t *current, clist_node_t *other)
{
	process_t *cur_proc = container_of(current, process_t, wait_node);
	process_t *other_proc = container_of(other, process_t, wait_node);
	return (get_wake_up(cur_proc) > get_wake_up(other_proc));
}

/**
 * @brief Insert a process in the sleeping queue, sorted by wake up time.
 *
 * @param proc Process to insert.
 */
static void _insert_sleep(process_t *proc)
{
	clist_node_t *node = get_sleep_node(proc);
	clist_insert_sorted(&sleeping_head, node, _wake_up_cmp);
}

void remove_from_sleeping(process_t *proc)
{
	clist_remove(get_sleep_node(proc));
}

void scheduler_sleep(uint32_t nbr_secs)
{
	irq_flags_t flags = irq_save();

	process_t *proc = get_active();
	_ready_queue_remove(proc);
	process_sleep(proc, nbr_secs);
	_insert_sleep(proc);
	_switch_out_active();
	irq_restore(flags);
}

/**
 * @brief Wake up policy applied on each item of the sleeping queue.
 *
 * @param node Sleeping node of the process.
 * @param arg Current time, in secondes.
 * @return -1 to break the for each iteration on the sorted list, 0 else.
 */
static inline int _wake_up_sleeping_cb(clist_node_t *node, void *arg)
{
	uint32_t now = *(uint32_t *)arg;
	process_t *proc = container_of(node, process_t, sleep_node);
	if (get_wake_up(proc) > now)
		return 0;

	clist_remove(node);
	scheduler_ready_process(proc);
	return 0;
}

void scheduler_wake_sleeping()
{
	irq_flags_t flags = irq_save();

	uint32_t now = seconds();
	clist_for_each(&sleeping_head, _wake_up_sleeping_cb, &now);

	irq_restore(flags);
}

/**
 * @brief Move a process from its ready queue to a wait queue.
 *
 * @param proc Process to block.
 * @param wq Wait queue to block it on.
 */
static inline void _move_to_wq(process_t *proc, wait_queue_t *wq)
{
	_ready_queue_remove(proc);
	process_block(proc);
	wq_enqueue(wq, &proc->wait_node);
}

void scheduler_block_on(wait_queue_t *wq)
{
	irq_flags_t flags = irq_save();

	process_t *proc = get_active();
	_move_to_wq(proc, wq);
	_switch_out_active();

	irq_restore(flags);
}

void scheduler_block_on_with_timeout(wait_queue_t *wq, uint32_t timeout_secs)
{
	irq_flags_t flags = irq_save();

	process_t *proc = get_active();
	_move_to_wq(proc, wq);
	if (timeout_secs > 0) {
		process_sleep(proc, timeout_secs);
		_insert_sleep(proc);
	}
	_switch_out_active();

	irq_restore(flags);
}

/**
 * @brief Wake up policy applied to a wait queue.
 *
 * @param current Node to compute on.
 * @param args Additional args, unused.
 * @return 0 (invariant).
 */
static inline int _wake_up_waiting_cb(clist_node_t *current, void *args)
{
	// args is unused but needed for the for a for each callback
	(void)args;
	// Wake up the process from the waiting queue
	scheduler_ready_process(container_of(current, process_t, wait_node));
	return 0;
}

void scheduler_wake_waiting_queue(wait_queue_t *wq)
{
	irq_flags_t flags = irq_save();
	wq_for_each_and_del(wq, _wake_up_waiting_cb, NULL);
	irq_restore(flags);
}

void init_scheduler_queues()
{
	_init_ready_queues();
	_init_sleep_queue();
}
