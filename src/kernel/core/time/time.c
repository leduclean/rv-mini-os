#include "time.h"
#include "console.h"
#include "minilib/stdio.h"
#include "platform.h"
#include "scheduler.h"

/** @brief Irq frequency, for a 1/20 s period. */
#define ITFREQ 20
#define DELAY (TIMER_FREQ / ITFREQ)

/** @brief Global irq tick counter. */
static uint32_t ticks;
static uint32_t prev;

static inline void _enable_sstc_extension()
{
	__asm__ __volatile__("csrs menvcfg, %0" ::"r"(MENVCFG_STCE));
}

static inline void _enable_stimecmp()
{
	__asm__ volatile("csrs mcounteren, %0" ::"r"(MCOUNTEREN_TM));
}

static inline void _enable_s_timer()
{
	__asm__("csrs sie, %0" ::"r"(1 << S_IRQ_TMR));
}

/** @brief Arm the supervisor timer comparator for the next tick. */
static inline void _update_timer()
{
	long t;
	__asm__ __volatile__("csrr %0, time" : "=r"(t));
	__asm__ __volatile__("csrw stimecmp, %0" ::"r"(t + DELAY));
}

inline uint32_t seconds()
{
	return ticks / ITFREQ;
}

void init_timer()
{
	_enable_sstc_extension();
	_enable_s_timer();
	_enable_stimecmp();

	ticks = 0;
	prev = -1;
	_update_timer();
}

void timer_irq_handler(void)
{
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
	_update_timer();
}
