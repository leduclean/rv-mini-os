#include "time.h"
#include "console.h"
#include "include/mmio.h"
#include "platform.h"
#include <stdint.h>
#include <stdio.h>

// IT FREQ for a 1 s period
#define ITFREQ 20
#define DELAY (TIMER_FREQ / ITFREQ)

extern void mon_traitant(void);

// Gloabl irq compteur
static uint32_t tirqcnt = 0;
static uint32_t previous_s = -1;

inline uint32_t nbr_secondes() { return tirqcnt / ITFREQ; }

void trap_handler(uint64_t mcause, uint64_t mie, uint64_t mip) {
  // Ignore the bit 63
  mcause &= ~(1ULL << 63);
  // Check if timer interrupt is enable
  if ((mie & (1 << IRQ_M_TMR)) && (mcause == 7)) {
    // Treat the timer interupt
    tirqcnt++;
    uint32_t s = nbr_secondes();

    // We only display when seconds changes
    if (s != previous_s) {
      uint32_t m = (s / 60) % 60;
      uint32_t h = s / 3600;

      char buf[16];
      sprintf(buf, "[%02d:%02d:%02d]", h, m, s % 60);
      display_top_right(buf, 10);
      previous_s = s;
    };
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

  init_traitant(mon_traitant);
}

void disable_timer() { __asm__("csrc mie, %0" ::"r"(IRQ_M_TMR)); }
