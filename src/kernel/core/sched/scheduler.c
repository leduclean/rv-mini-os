#include <stddef.h>
#include <stdint.h>

#include <lib/clist.h>
#include <lib/container.h>
#include <lib/stdio.h>

#include <arch/cpu.h>
#include <arch/trap.h>

#include <kernel/mmap.h>
#include <kernel/task.h>
#include <kernel/scheduler.h>
#include <kernel/spinlock.h>
#include <kernel/time.h>
#include <kernel/vpages.h>
#include <kernel/waitqueue.h>

/**
 * @brief This counter is used to disable the scheduler preemption.
 */
static uint32_t preempt_count;

/**
 * @brief Flag indicated a delayed schedule operation.
 */
static bool need_resched;

extern void ctx_sw(ctx_t *old_ctx, ctx_t *new_ctx);

/** @brief Priority mapped ready queues. */
static clist_node_t ready_queues[PRIORITY_COUNT];

/** @brief Highest ready priority. */
static priority highest_ready_tracking;

/** @brief Tasks sleeping on a timer, sorted by wake up date. */
static clist_node_t sleeping_head;

static inline bool preempt_disabled(void)
{
	return preempt_count > 0;
}

static inline void delay_schedule(void)
{
	need_resched = true;
}

/** @brief Init all the ready queues and the priority flag. */
static void init_ready_queues(void)
{
	// Reset all the node of the ready queue
	for (int i = 0; i < PRIORITY_COUNT; i++) {
		clist_node_t *current_head = &ready_queues[i];
		clist_init_node(current_head);
	}
	// The initiate prioity should be IDLE
	highest_ready_tracking = IDLE;
}

/** @brief Init the sleeping queue. */
static void init_sleep_queue(void)
{
	clist_init_node(&sleeping_head);
}

/**
 * @brief Add a task at the end of its corresponding priority ready queue.
 *
 * @param task Task to enqueue.
 */
static void ready_queue_enqueue(task_t *task)
{
	priority prior_class = task->priority;
	clist_node_t *rq = &ready_queues[prior_class];
	clist_push_back(rq, &task->ready_node);
}

/**
 * @brief Remove a task from the ready queue it is in.
 *
 * @param task Task to remove.
 */
static inline void ready_queue_remove(task_t *task)
{
	if (!clist_is_in_list(&task->ready_node))
		return;
	clist_remove(&task->ready_node);
}

/**
 * @brief Round robin circular rotation of a ready queue.
 *
 * @note This function also gives the next task to switch on.
 *
 * @param rq Ready queue to rotate on.
 * @return Pointer to the next task.
 */
static task_t *rotate_ready_queue(clist_node_t *rq)
{
	// Nothing to do if the queue is empty or there is only one element
	if (clist_empty(rq) || rq->next->next == rq)
		return container_of(clist_first(rq), task_t, ready_node);
	clist_node_t *first = clist_pop_front(rq);
	clist_push_back(rq, first);

	return container_of(clist_first(rq), task_t, ready_node);
}

/**
 * @brief Pick the highest priority ready queue in the ready queues.
 *
 * @return Pointer to the highest priority queue (clist).
 */
static inline clist_node_t *pick_highest(void)
{
	return &ready_queues[highest_ready_tracking];
}

/**
 * @brief Refresh the highest priority flag, scanning all the queues.
 *
 * @note The scan is done incrementing the index, since the highest priority
 * is 0.
 */
static void refresh_highest_prio(void)
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
 * @brief Check if a task should change the highest priority flag.
 *
 * @param task Task to check on.
 */
static void update_highest(task_t *task)
{
	priority prior_class = task->priority;
	if (priority_higher(prior_class, highest_ready_tracking)) {
		highest_ready_tracking = prior_class;
	}
}

/**
 * @brief Main rescheduling function, changing the context between tasks.
 *
 * @param next Task to switch on.
 */
static void do_ctx_switch(task_t *next)
{
	ctx_t *old_ctx = &task_active()->ctx;
	task_switch_active(next);
	ctx_sw(old_ctx, &next->ctx);
}

/**
 * @brief Check if a priority preemption is needed and possible and does it if needed.
 *
 * @param task Task that may preempt the active one.
 */
static void check_and_preempt(task_t *task)
{

	task_t *current = task_active();

	if (!current) {
		return;
	}

	priority prior = task->priority;
	priority current_prior = current->priority;
	if (priority_higher(prior, current_prior)) {
		if (preempt_disabled()) {
			delay_schedule();
		} else {
			// Immediatly switch to this task
			do_ctx_switch(task);
		}
	}
}

/**
 * @brief Switch out the active task on its inactivity.
 *
 * @note Called on a terminaison or on a sleep, it switches on the head of
 * the highest priority ready queue.
 */
static void switch_out_active(void)
{
	irq_flags_t flags = irq_save();

	refresh_highest_prio();
	clist_node_t *next_node = clist_first(pick_highest());
	do_ctx_switch(container_of(next_node, task_t, ready_node));

	irq_restore(flags);
}

/**
 * @brief Sort policy of the sleeping queue.
 *
 * @param current Node already in the queue.
 * @param other Node being inserted.
 * @return 1 if @p current wakes up after @p other, 0 otherwise.
 */
static inline int wake_up_cmp(clist_node_t *current, clist_node_t *other)
{
	task_t *cur_task = container_of(current, task_t, sleep_node);
	task_t *other_task = container_of(other, task_t, sleep_node);
	return (cur_task->wake_up_time > other_task->wake_up_time);
}

/**
 * @brief Insert a task in the sleeping queue, sorted by wake up time.
 *
 * @param task Task to insert.
 */
static void insert_sleep(task_t *task)
{
	clist_node_t *node = &task->sleep_node;
	clist_insert_sorted(&sleeping_head, node, wake_up_cmp);
}

/**
 * @brief Wake up policy applied on each item of the sleeping queue.
 *
 * @param node Sleeping node of the task.
 * @param arg Current time, in secondes.
 * @return -1 to break the for each iteration on the sorted list, 0 else.
 */
static inline int wake_up_sleeping_cb(clist_node_t *node, void *arg)
{
	uint32_t now = *(uint32_t *)arg;
	task_t *task = container_of(node, task_t, sleep_node);
	if (task->wake_up_time > now)
		return 0;

	clist_remove(node);
	scheduler_ready_task(task);
	return 0;
}

/**
 * @brief Move a task from its ready queue to a wait queue.
 *
 * @param task Task to block.
 * @param wq Wait queue to block it on.
 */
static inline void move_to_wq(task_t *task, wait_queue_t *wq)
{
	ready_queue_remove(task);
	task_block(task);
	wq_enqueue(wq, &task->wait_node);
}

/**
 * @brief Wake up policy applied to a wait queue.
 *
 * @param current Node to compute on.
 * @param args Additional args, unused.
 * @return 0 (invariant).
 */
static inline int wake_up_waiting_cb(clist_node_t *current, void *args)
{
	// args is unused but needed for the for a for each callback
	(void)args;
	// Wake up the task from the waiting queue
	scheduler_ready_task(container_of(current, task_t, wait_node));
	return 0;
}

void scheduler_admit(task_t *task)
{
	irq_flags_t flags = irq_save();

	ready_queue_enqueue(task);
	update_highest(task);
	check_and_preempt(task);

	irq_restore(flags);
}

void scheduler_disable_preempt(void)
{
	preempt_count++;
}

void scheduler_enable_preempt(void)
{
	preempt_count--;
	if (preempt_count == 0 && need_resched) {
		need_resched = false;
		scheduler_rotate();
	}
}

void scheduler_rotate(void)
{
	if (preempt_count > 0) {
		// Preempt disable -> skip
		return;
	}

	irq_flags_t flags = irq_save();

	task_t *next = rotate_ready_queue(pick_highest());
	if (next != task_active())
		do_ctx_switch(next);

	irq_restore(flags);
}

void scheduler_ready_task(task_t *task)
{
	irq_flags_t flags = irq_save();

	// Wake the task
	task_wake(task);

	// Update the highest if needed
	update_highest(task);

	// Insert in the correct run queue
	ready_queue_enqueue(task);

	// Preempt if needed
	check_and_preempt(task);
	irq_restore(flags);
}

void scheduler_exit(int exit_code)
{
	//TODO: check if it properly restores irqs after.
	(void)irq_save();

	task_t *current = task_active();

	task_zombify(current, exit_code);
	switch_out_active();
	panic("Zombie exit");
}

static inline int exit_tgid_cb(clist_node_t *node, void *arg)
{
	uint8_t tgid = *(uint8_t *)arg;

	task_t *current = task_active();
	task_t *p = container_of(node, task_t, task_node);

	//TODO: On multicore this is gonna crash.
	// maybe a flag for killed needs to be add.
	if (tgid == p->tgid && p != current) {
		// No zombie state, we directly destroy the thread
		task_destroy(p);
	}

	return 0;
}

void scheduler_exit_group(int exit_code)
{
	//TODO: check if it properly restores irqs after.
	(void)irq_save();

	task_t *task = task_active();
	clist_for_each(task_table_clist(), exit_tgid_cb, &task->tgid);
	scheduler_exit(exit_code);
}

void scheduler_sleep(uint32_t nbr_secs)
{
	irq_flags_t flags = irq_save();

	task_t *task = task_active();
	ready_queue_remove(task);
	task_sleep(task, nbr_secs);
	insert_sleep(task);
	switch_out_active();
	irq_restore(flags);
}

void scheduler_wake_sleeping(void)
{
	irq_flags_t flags = irq_save();

	uint32_t now = time_seconds();
	clist_for_each(&sleeping_head, wake_up_sleeping_cb, &now);

	irq_restore(flags);
}

void scheduler_block_on(wait_queue_t *wq)
{
	irq_flags_t flags = irq_save();

	task_t *task = task_active();
	move_to_wq(task, wq);
	switch_out_active();

	irq_restore(flags);
}

void scheduler_block_on_locked(wait_queue_t *wq, spinlock_t *s)
{
	irq_flags_t flags = irq_save();
	task_t *task = task_active();

	move_to_wq(task, wq);
	spinlock_unlock(s);
	switch_out_active();

	irq_restore(flags);
}

void scheduler_block_on_with_timeout(wait_queue_t *wq, uint32_t timeout_secs)
{
	irq_flags_t flags = irq_save();

	task_t *task = task_active();
	move_to_wq(task, wq);
	if (timeout_secs > 0) {
		task_sleep(task, timeout_secs);
		insert_sleep(task);
	}
	switch_out_active();

	irq_restore(flags);
}

void scheduler_wake_waiting_queue(wait_queue_t *wq)
{
	irq_flags_t flags = irq_save();
	wq_for_each_and_del(wq, wake_up_waiting_cb, NULL);
	irq_restore(flags);
}

void scheduler_init(void)
{
	init_ready_queues();
	init_sleep_queue();
}
