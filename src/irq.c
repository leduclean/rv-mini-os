#include "mmio.h"
#include "platform.h"
#include "time.h"
#include "uart.h"
#include <stdint.h>
#include <stdio.h>

/** Enable timer irq and trigger the timer **/
void enable_timer() {
  // Clock configuration
  init_timer();

  // Timer irq support
  __asm__("csrs mie, %0" ::"r"(1 << IRQ_M_TMR));
}

/** Disable the timer irq **/
void disable_timer() { __asm__("csrc mie, %0" ::"r"(1 << IRQ_M_TMR)); }

/** Enable machine external irq **/
void enable_external() { __asm__("csrs mie, %0" ::"r"(1 << IRQ_M_EXT)); }

/** Disable Machine external interrupt **/
void disable_external() { __asm__("csrc mie, %0" ::"r"(1 << IRQ_M_EXT)); }

/* Set a priority for a specific irq device
 * The range of priority is between 0 and 7.
 * 0 means (never interrupt).
 * */
static void plic_set_pty(uint32_t irq_id, uint32_t priority) {
  if (priority > 7) {
    // range is between 0 and 7.
    priority = 7;
  }
  MMIO32(PLIC_SOURCE + (irq_id << 2)) = priority;
}

/* Set a priority treshold
 * Supports 7 levels of priority for example, a threshold value of zero permits
 * all interrupts with non-zero priority, whereas a value of 7 masks all
 * interrupts.
 */
static void set_priority_treshold(uint32_t threshold) {
  if (threshold > 7) {
    threshold = 7;
  }
  MMIO32(PLIC_TARGET) = threshold;
}

/* Config the plic for uart irq */
void plic_uart_config() {
  /* Enable UART irq */
  MMIO32(PLIC_ENABLE) |= PLIC_ENABLE_UART;
  /* Set Uart priority to  3 */
  plic_set_pty(PLIC_UART_ID, 3);
  /* Set threshold to 0 to enable all < 0 interrupts */
  set_priority_treshold(0);
}

/* Claim the interrupt from the plic */
static uint32_t claim_plic() { return MMIO32(PLIC_IRQ_CLAIM); }

/* Complete the interrupt */
static void complete_plic(uint32_t irq) { MMIO32(PLIC_IRQ_CLAIM) = irq; }

/* Treat the external irq interrupts */
static void external_irq_handler() {
  uint32_t irq = claim_plic();
  switch (irq) {
  case PLIC_UART_ID:
    uart_irq_handler();
    break;
  }
  complete_plic(irq);
}

/** Set the trigerred entry point called to treat irq **/
void init_trap_entry(void (*entry)()) {
  __asm__("csrw mtvec, %0" ::"r"(entry));
}

/** Trap Handler triggered by entry point **/
void trap_handler(uint64_t mcause, uint64_t mie, uint64_t mip) {
  // Ignore the bit 63
  mcause &= ~(1ULL << 63);
  if ((mie & (1 << IRQ_M_EXT)) && (mcause == IRQ_M_EXT)) {
    // Extern interrupt
    external_irq_handler();
  } else if ((mie & (1 << IRQ_M_TMR)) && (mcause == IRQ_M_TMR)) {
    // Timer interrupt
    timer_irq_handler();
  }
}
