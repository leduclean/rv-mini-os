/**
 * @file
 * @brief Priority based round robin scheduler.
 */

#pragma once
#include <stdint.h>

#include <kernel/task.h>
#include <kernel/spinlock.h>

/**
 * @brief Use this function before @scheduler_enable_preempt for a no switch section.
 * @warning @scheduler_enable_preempt should always be called after this function.
 */
void scheduler_disable_preempt(void);

/**
 * @brief Use this function after @scheduler_disable_preempt for a no switch section.
 * @warning @scheduler_disable_preempt should always be called before this function.
 */
void scheduler_enable_preempt(void);
/**
 * @brief Rotate the highest priority ready queue and switch to its head.
 *
 * @note Triggered by the timer irq.
 */
void scheduler_rotate(void);

/**
 * @brief Admit a task in its corresponding priority ready queue.
 *
 * @param task Task to admit.
 */
void scheduler_admit(task_t *task);

/**
 * @brief Wake a task up and reschedule it in its ready queue.
 *
 * @param task Task to make ready.
 */
void scheduler_ready_task(task_t *task);

/** @brief Wake every sleeping task whose wake up time is reached. */
void scheduler_wake_sleeping(void);

/**
 * @brief Wake every task blocked on a wait queue.
 *
 * This function is supposed to be called on an event, to release the
 * tasks blocked on this event.
 *
 * @param wq Wait queue associated with the block condition.
 */
void scheduler_wake_waiting_queue(wait_queue_t *wq);

/**
 * @brief Block the active task on a wait queue.
 *
 * @param wq Wait queue to block on.
 */
void scheduler_block_on(wait_queue_t *wq);

/**
 * @brief Block the active task on a wait queue guarded by a 
 * lock.
 *
 * @param wq Wait queue to block on.
 * @param s The associated lock of the queue.
 *
 * @warning @s MUST be locked by @spinlock_lock or @spinlock_lock_irq_save before calling this.
 * @note This function will handle the release of the lock.
 */
void scheduler_block_on_locked(wait_queue_t *wq, spinlock_t *s);

/**
 * @brief Block the active task on a wait queue, with a timeout.
 *
 * @param wq Wait queue to block on.
 * @param timeout_secs Timeout in secondes, 0 to block without timeout.
 */
void scheduler_block_on_with_timeout(wait_queue_t *wq, uint32_t timeout_secs);

/** @brief Init the ready queues and the sleeping queue. */
void scheduler_init(void);

/**
 * @brief Set the active task to sleeping state.
 *
 * @param nbr_secondes Sleeping duration, in secondes.
 */
void scheduler_sleep(uint32_t nbr_secondes);

/** @brief Exit the active task and switch to the next ready one.
 *
 * @note This will implicitly cause the current task to be zombify.
 *
 * @param exit_code Exit code of the exited task.
 */
[[noreturn]] void scheduler_exit(int exit_code);

/**
 * @brief Exit the active task and all the thread with the same @gtid 
 *
 * @param exit_code Exit code of the exited task.
 */
[[noreturn]] void scheduler_exit_group(int exit_code);
