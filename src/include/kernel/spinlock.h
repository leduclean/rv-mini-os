#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef struct spinlock spinlock_t;

/**
 * @brief Init a spinlock structure.
 *
 * @param s A pointer to the spinlock.
 */
void spinlock_init(spinlock_t *s);

/**
 * @brief Blocking acquire primitive to a spinlock.
 *
 * @param s A pointer to the spinlock structure.
 */
void spinlock_acquire(spinlock_t *s);

/**
 * @brief Release an acquired spinlock acquired by the @spinlock_acquire.
 *
 * @param s A pointer to the spinlock.
 */
void spinlock_release(spinlock_t *s);
