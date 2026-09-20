#pragma once
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>

#include "asm/cpu.h"

typedef struct {
	// We use uint because riscv atomics use word or double word.
	// i.e "A" extension and amoswap instruction.
	atomic_uint flag;
} spinlock_t;

#define SPINLOCK_INIT { .flag = 0 }

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
void spinlock_lock(spinlock_t *s);

/**
 * @brief Release an acquired spinlock acquired by the @spinlock_acquire.
 *
 * @param s A pointer to the spinlock.
 */
void spinlock_unlock(spinlock_t *s);

/**
 * @brief Acquire a spinlock and save irq state.
 *
 * @param s A pointer to the spinlock.
 * @return The saved irq flags.
 */
static inline irq_flags_t spinlock_lock_irq_save(spinlock_t *s)
{
	irq_flags_t flags = irq_save();
	spinlock_lock(s);
	return flags;
}

/**
 * @brief Release a spinlock and restore irq state.
 *
 * @param s A pointer to the spinlock.
 * @param flags The saved irq flags.
 */
static inline void spinlock_unlock_irq_restore(spinlock_t *s, irq_flags_t flags)
{
	spinlock_unlock(s);
	irq_restore(flags);
}
