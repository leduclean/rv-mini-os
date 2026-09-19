#pragma once
#include <stdbool.h>
#include <stdint.h>

#include "asm/cpu.h"

typedef struct {
	volatile unsigned int flag;
} spinlock_t;

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

/**
 * @brief Acquire a spinlock and save irq state.
 *
 * @param s A pointer to the spinlock.
 * @return The saved irq flags.
 */
static inline irq_flags_t spinlock_acquire_irq_save(spinlock_t *s)
{
	irq_flags_t flags = irq_save();
	spinlock_acquire(s);
	return flags;
}

/**
 * @brief Release a spinlock and restore irq state.
 *
 * @param s A pointer to the spinlock.
 * @param flags The saved irq flags.
 */
static inline void spinlock_release_irq_restore(spinlock_t *s,
						irq_flags_t flags)
{
	spinlock_release(s);
	irq_restore(flags);
}
