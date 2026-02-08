#pragma once
#include "arch/riscv/platform.h"
/* Met le processeur en pause en attente d'une interruption */
inline static void hlt() { __asm__ __volatile__("wfi" ::: "memory"); }

/* Autorise les interruptions au niveau du processeur */
inline static void enable_it() {
  __asm__("csrs mstatus, %0" ::"i"(MSTATUS_MIE));
}

/* Interdit les interruptions au niveau du processeur */
inline static void disable_it() {
  __asm__("csrc mstatus, %0" ::"r"(MSTATUS_MIE));
}
typedef unsigned long irq_flags_t;

/** Disable the cpu irq and save the previous state */
static inline irq_flags_t irq_save() {
  irq_flags_t flags;
  __asm__("csrr %0, mstatus" : "=r"(flags));
  if (flags & MSTATUS_MIE) {
    disable_it();
  }
  return flags;
}

inline static void irq_restore(irq_flags_t flags) {
  // Interupt were enabled so we restore them
  if (flags & MSTATUS_MIE)
    enable_it();
}
