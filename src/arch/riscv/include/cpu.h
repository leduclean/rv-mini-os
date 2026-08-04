/**
 * @file
 * @brief Cpu level interrupt control.
 */

#pragma once
#include "platform.h"

/** @brief Put the cpu in pause, waiting for an interrupt. */
inline static void hlt() { __asm__ __volatile__("wfi" ::: "memory"); }

/** @brief Enable the interrupts at the cpu level. */
inline static void enable_it() {
  __asm__("csrs mstatus, %0" ::"i"(MSTATUS_MIE));
}

/** @brief Disable the interrupts at the cpu level. */
inline static void disable_it() {
  __asm__("csrc mstatus, %0" ::"r"(MSTATUS_MIE));
}

/** @brief Saved interrupt state, as returned by irq_save(). */
typedef unsigned long irq_flags_t;

/**
 * @brief Disable the cpu irq and save the previous state.
 *
 * @return Previous mstatus, to give back to irq_restore().
 */
static inline irq_flags_t irq_save() {
  irq_flags_t flags;
  __asm__("csrr %0, mstatus" : "=r"(flags));
  if (flags & MSTATUS_MIE) {
    disable_it();
  }
  return flags;
}

/**
 * @brief Restore the cpu irq state saved by irq_save().
 *
 * @param flags State returned by the matching irq_save() call.
 */
inline static void irq_restore(irq_flags_t flags) {
  // Interupt were enabled so we restore them
  if (flags & MSTATUS_MIE)
    enable_it();
}
