#include "time.h"
#include "console.h"
#include "minilib/stdio.h"
#include "mmio.h"
#include "platform.h"
#include "scheduler.h"

// IT FREQ for a 1/20 s period
#define ITFREQ 20
#define DELAY (TIMER_FREQ / ITFREQ)

// Gloabl irq tick counter
static uint32_t ticks;
static uint32_t prev;

inline uint32_t seconds() { return ticks / ITFREQ; }

static inline void update_timer() {
  MMIO64(CLINT_TIMER_CMP) = MMIO64(CLINT_TIMER) + DELAY;
}

void init_timer() {
  ticks = 0;
  prev = -1;
  update_timer();
}

void timer_irq_handler(void) {
  // Treat the timer interupt
  ticks++;
  uint32_t s = seconds();

  // We only display when seconds changes
  if (s != prev) {
    uint32_t m = (s / 60) % 60;
    uint32_t h = s / 3600;

    char buf[16];
    sprintf(buf, "[%02d:%02d:%02d]", h, m, s % 60);
    display_top_right(buf, 10);
    prev = s;
  };

  scheduler_wake_sleeping();
  scheduler_rotate();
  update_timer();
}
