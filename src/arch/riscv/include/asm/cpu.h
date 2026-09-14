/**
 * @file
 * @brief Cpu level interrupt control.
 */

#pragma once

#include <lib/stdio.h>

#include <asm/csr.h>
#include <asm/platform.h>

/** @brief Put the cpu in pause, waiting for an interrupt. */
inline static void hlt(void)
{
	__asm__ __volatile__("wfi" ::: "memory");
}

/** @brief Enable the interrupts at the machine level. */
inline static void irq_enable_m(void)
{
	csr_set(mstatus, MSTATUS_MIE);
}

/** @brief Disable the interrupts at the machine level. */
inline static void irq_disable_m(void)
{
	csr_clear(mstatus, MSTATUS_MIE);
}

/** @brief Enable the interrupts at the supervisor level. */
inline static void irq_enable_s(void)
{
	csr_set(sstatus, SSTATUS_SIE);
}

/** @brief Disable the interrupts at the supervisor level. */
inline static void irq_disable_s(void)
{
	csr_clear(sstatus, SSTATUS_SIE);
}

/**
 * @brief Report @msg with its call site, then halt the cpu for good.
 *
 * @note Never call _panic() directly, use the panic() macro: __FILE__ and
 * __LINE__ must expand at the call site, not here.
 */
__attribute__((noreturn)) void _panic(const char *msg, const char *file,
				      int line);

/** @brief Halt the kernel, reporting @msg and where it was raised. */
#define panic(msg) _panic((msg), __FILE__, __LINE__)

/** @brief Saved interrupt state, as returned by irq_save(). */
typedef unsigned long irq_flags_t;

/**
 * @brief Disable the cpu irq and save the previous state.
 *
 * @return Previous sstatus, to give back to irq_restore().
 */
static inline irq_flags_t irq_save(void)
{
	irq_flags_t flags = csr_read(sstatus);
	if (flags & SSTATUS_SIE) {
		irq_disable_s();
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
		irq_enable_s();
}
