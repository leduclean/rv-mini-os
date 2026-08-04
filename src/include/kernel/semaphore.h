/**
 * @file
 * @brief Counting semaphore.
 */

#pragma once

#include "minilib/stdint.h"

/** @brief Counting semaphore, opaque to its users. */
typedef struct semaphore semaphore_t;

/**
 * @brief Init a semaphore.
 *
 * @param sem Semaphore to init.
 * @param val Initial number of available ressources.
 */
void sem_init(semaphore_t *sem, int val);

/**
 * @brief Release a ressource and wake the first waiting process, if any.
 *
 * @param sem Semaphore to release.
 */
void sem_release(semaphore_t *sem);

/**
 * @brief Take a ressource without blocking.
 *
 * @param sem Semaphore to take.
 * @return 0 if a ressource was taken, -1 if none was available.
 */
int8_t sem_try_take(semaphore_t *sem);

/**
 * @brief Take a ressource, blocking until one is available.
 *
 * @param sem Semaphore to take.
 */
void sem_take(semaphore_t *sem);
