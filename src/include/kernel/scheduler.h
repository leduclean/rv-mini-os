/**
 * @file
 * @brief Priority based round robin scheduler.
 */

#pragma once
#include <stdint.h>

#include <kernel/process.h>

#include "asm/cpu.h"
#include "kernel/spinlock.h"

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
 * @brief Admit a process in its corresponding priority ready queue.
 *
 * @param proc Process to admit.
 */
void scheduler_admit(process_t *proc);

/**
 * @brief Wake a process up and reschedule it in its ready queue.
 *
 * @param proc Process to make ready.
 */
void scheduler_ready_process(process_t *proc);

/** @brief Wake every sleeping process whose wake up time is reached. */
void scheduler_wake_sleeping(void);

/**
 * @brief Wake every process blocked on a wait queue.
 *
 * This function is supposed to be called on an event, to release the
 * processes blocked on this event.
 *
 * @param wq Wait queue associated with the block condition.
 */
void scheduler_wake_waiting_queue(wait_queue_t *wq);

/**
 * @brief Remove a process from the sleeping queue.
 *
 * @param proc Process to remove.
 */
void scheduler_remove_sleeping(process_t *proc);

/**
 * @brief Check if a process is in the sleeping queue.
 *
 * @param proc Process to check.
 * @return 1 if it is sleeping, 0 otherwise.
 */
uint8_t scheduler_is_sleeping(process_t *proc);

/**
 * @brief Block the active process on a wait queue.
 *
 * @param wq Wait queue to block on.
 */
void scheduler_block_on(wait_queue_t *wq);

/**
 * @brief Block the active process on a wait queue guarded by a 
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
 * @brief Block the active process on a wait queue, with a timeout.
 *
 * @param wq Wait queue to block on.
 * @param timeout_secs Timeout in secondes, 0 to block without timeout.
 */
void scheduler_block_on_with_timeout(wait_queue_t *wq, uint32_t timeout_secs);

/** @brief Init the ready queues and the sleeping queue. */
void scheduler_init(void);

/**
 * @brief Set the active process to sleeping state.
 *
 * @param nbr_secondes Sleeping duration, in secondes.
 */
void scheduler_sleep(uint32_t nbr_secondes);

/** @brief Terminate the active process and switch to the next ready one. 
 *
 * @param exit_code Exit code of the process to terminate.
 */
void scheduler_terminate(int exit_code);
