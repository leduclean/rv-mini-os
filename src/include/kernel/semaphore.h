/**
 * @file
 * @brief Counting semaphore.
 */

#pragma once

#include <stdint.h>

#include <kernel/spinlock.h>
#include <kernel/waitqueue.h>

/** @brief Counting semaphore and the processes waiting on it. */
typedef struct semaphore {
	int count; ///< Number of available ressources.
	wait_queue_t wq; ///< Processes blocked waiting for a ressource.
	spinlock_t
		lock; ///< Lock protecting the inner structure of the semaphore.
} semaphore_t;

#define SEMAPHORE_INITIALIZER(name, val)           \
	{ .count = (val),                          \
	  .wq = WAIT_QUEUE_INITIALIZER((name).wq), \
	  .lock = SPINLOCK_UNLOCKED }

/**
 * @brief Init a semaphore.
 *
 * @param sem Semaphore to init.
 * @param val Initial number of available ressources.
 */
void sem_init(semaphore_t *sem, int val);

/**
 * @brief Post a ressource and wake the first waiting process, if any.
 *
 * @param sem Semaphore to release.
 */
void sem_post(semaphore_t *sem);

/**
 * @brief Take a ressource without blocking.
 *
 * @param sem Semaphore to take.
 * @return 0 if a ressource was taken, -1 if none was available.
 */
int sem_try_take(semaphore_t *sem);

/**
 * @brief Wait for a ressource, blocking until one is available.
 *
 * @param sem Semaphore to take.
 */
void sem_wait(semaphore_t *sem);
