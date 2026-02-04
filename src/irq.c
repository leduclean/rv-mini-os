#include "platform.h"
#include "time.h"

/** Set the trigerred entry point called to treat irq **/
void init_trap_entry(void (*entry)()) {
  __asm__("csrw mtvec, %0" ::"r"(entry));
}

/** Trap Handler triggered by entry point **/
void trap_handler(uint64_t mcause, uint64_t mie, uint64_t mip) {
  // Ignore the bit 63
  mcause &= ~(1ULL << 63);
  // Check if timer interrupt is enable
  if ((mie & (1 << IRQ_M_TMR)) && (mcause == 7)) {
    timer_irq_handler();
  }
}

/** Enable timer irq and trigger the timer **/
void enable_timer() {
  // Clock configuration
  init_timer();

  // Timer irq support
  __asm__("csrs mie, %0" ::"r"(1 << IRQ_M_TMR));
}

/** Disable the timer irq **/
void disable_timer() { __asm__("csrc mie, %0" ::"r"(IRQ_M_TMR)); }
