/**
 * @file
 * @brief Cpu level interrupt control.
 */

#pragma once

#include "csr.h"
#include "platform.h"

/** @brief Put the cpu in pause, waiting for an interrupt. */
inline static void hlt()
{
	__asm__ __volatile__("wfi" ::: "memory");
}

/** @brief Enable the interrupts at the machine level. */
inline static void enable_m_irq()
{
	csr_set(mstatus, MSTATUS_MIE);
}

/** @brief Disable the interrupts at the machine level. */
inline static void disable_m_irq()
{
	csr_clear(mstatus, MSTATUS_MIE);
}

/** @brief Enable the interrupts at the supervisor level. */
inline static void enable_s_irq()
{
	csr_set(sstatus, SSTATUS_SIE);
}

/** @brief Disable the interrupts at the supervisor level. */
inline static void disable_s_irq()
{
	csr_clear(sstatus, SSTATUS_SIE);
}

/** @brief Saved interrupt state, as returned by irq_save(). */
typedef unsigned long irq_flags_t;

/**
 * @brief Disable the cpu irq and save the previous state.
 *
 * @return Previous sstatus, to give back to irq_restore().
 */
static inline irq_flags_t irq_save()
{
	irq_flags_t flags = csr_read(sstatus);
	if (flags & SSTATUS_SIE) {
		disable_s_irq();
	}
	return flags;
}

/**
 * @brief Restore the cpu irq state saved by irq_save().
 *
 * @param flags State returned by the matching irq_save() call.
 */
inline static void irq_restore(irq_flags_t flags)
{
	// Interupt were enabled so we restore them
	if (flags & SSTATUS_SIE)
		enable_s_irq();
}
