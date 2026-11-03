/**
 * @file
 * @brief Blocking mutex with ownership tracking.
 */

#pragma once
#include <stdint.h>

#include <kernel/task.h>
#include <kernel/spinlock.h>

/** @brief Mutex, with its owner and the tasks waiting for it. */
typedef struct mutex {
	uint8_t locked; ///< 1 if the mutex is held, 0 otherwise.
	task_t *owner; ///< Task holding the mutex, NULL if free.
	wait_queue_t wq; ///< Tasks blocked waiting for the mutex.
	spinlock_t lock; ///< Mutex internal structure lock.
} mutex_t;

#define MUTEX_INITIALIZER(name)                   \
	{ .locked = 0,                             \
	  .owner = NULL,                           \
	  .wq = WAIT_QUEUE_INITIALIZER((name).wq), \
	  .lock = SPINLOCK_UNLOCKED }

/**
 * @brief Init a mutex in the unlocked state.
 *
 * @param m Mutex to init.
 */
void mutex_init(mutex_t *m);

/**
 * @brief Give the lock state of the mutex.
 *
 * @param m Mutex to read.
 * @return 1 if locked, 0 otherwise.
 */
uint8_t mutex_is_lock(const mutex_t *m);

/**
 * @brief Give the owner of the mutex.
 *
 * @param m Mutex to read.
 * @return Task owning @p m, NULL if it is free.
 */
task_t *mutex_owner(const mutex_t *m);

/**
 * @brief Take the ownership of a mutex, blocking until it is available.
 *
 * @param m Mutex to lock.
 */
void mutex_lock(mutex_t *m);

/**
 * @brief Try to take the ownership of a mutex without blocking.
 *
 * @param m Mutex to lock.
 * @retval 0 Ownership taken.
 * @retval -EBUSY Already locked.


 */
int mutex_trylock(mutex_t *m);

/**
 * @brief Try to take the ownership of a mutex with a timeout.
 *
 * @param m Mutex to lock.
 */
void mutex_trylock_timeout(mutex_t *m);

/**
 * @brief Release the ownership of a mutex.
 *
 * @note The ownership is handed over to the first waiting task, if any.
 * A task that is not the owner cannot unlock @p m.
 *
 * @param m Mutex to unlock.
 */
void mutex_unlock(mutex_t *m);
