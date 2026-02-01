#include "time.h"
#include "console.h"
#include "mmio.h"
#include "platform.h"
#include "scheduler.h"
#include <stdint.h>
#include <stdio.h>

// IT FREQ for a 1 s period
#define ITFREQ 20
#define DELAY (TIMER_FREQ / ITFREQ)

// Gloabl irq compteur
static uint32_t tirqcnt = 0;

inline uint32_t seconds() { return tirqcnt / ITFREQ; }

static void timer_interrupt_handler(void) {
  // Treat the timer interupt
  tirqcnt++;
  uint32_t s = seconds();

  uint32_t m = (s / 60) % 60;
  uint32_t h = s / 3600;

  char buf[16];
  sprintf(buf, "[%02d:%02d:%02d]", h, m, s % 60);
  display_top_right(buf, 10);

  // TODO implement wake up sleeping and then decomment
  //  wake_up_sleeping();
  scheduler_rotate();
}

void trap_handler(uint64_t mcause, uint64_t mie, uint64_t mip) {
  // Ignore the bit 63
  mcause &= ~(1ULL << 63);
  // Check if timer interrupt is enable
  if ((mie & (1 << IRQ_M_TMR)) && (mcause == 7)) {
    timer_interrupt_handler();
    // Relaunch another interupt
    MMIO64(CLINT_TIMER_CMP) = MMIO64(CLINT_TIMER) + DELAY;
  }
}

void init_traitant(void (*traitant)()) {
  __asm__("csrw mtvec, %0" ::"r"(traitant));
}

void enable_timer() {
  // Clock configuration
  MMIO64(CLINT_TIMER_CMP) = MMIO64(CLINT_TIMER) + DELAY;

  // Timer irq support
  __asm__("csrs mie, %0" ::"r"(1 << IRQ_M_TMR));
}

void disable_timer() { __asm__("csrc mie, %0" ::"r"(IRQ_M_TMR)); }
