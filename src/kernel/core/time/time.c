#include <lib/stdio.h>

#include <asm/csr.h>
#include <asm/platform.h>

#include <drivers/console.h>

#include <kernel/scheduler.h>
#include <kernel/semaphore.h>
#include <kernel/time.h>

/** @brief Irq frequency, for a 1/20 s period. */
#define ITFREQ 20
#define DELAY (TIMER_FREQ / ITFREQ)

/** @brief Global irq tick counter. */
static uint32_t ticks;

static inline void _enable_sstc_extension(void)
{
	csr_set(menvcfg, MENVCFG_STCE);
}

static inline void _enable_stimecmp(void)
{
	csr_set(mcounteren, MCOUNTEREN_TM);
}

static inline void _enable_s_timer(void)
{
	csr_set(sie, SIE_STIE);
}

/** @brief Arm the supervisor timer comparator for the next tick. */
static inline void _update_timer(void)
{
	unsigned long time = csr_read(time);
	csr_write(stimecmp, time + DELAY);
}

inline uint32_t time_seconds(void)
{
	return ticks / ITFREQ;
}

static semaphore_t clock = SEMAPHORE_INITIALIZER(clock, 0);
static semaphore_t tick = SEMAPHORE_INITIALIZER(tick, 0);

/**
 * @brief Dameon that handles the clock display
 */
static void _clock_daemon(void)
{
	for (;;) {
		sem_wait(&clock);

		uint32_t s = time_seconds();
		uint32_t m = (s / 60) % 60;
		uint32_t h = s / 3600;

		char buf[16];
		sprintf(buf, "[%02d:%02d:%02d]", h, m, s % 60);
		console_display_top_right(buf, 10);
	}
}

/**
 * @brief Kernel Daemon that handles a time tick ie wakes sleeping process
 * and preempt if needed.
 */
static void _tick_daemon(void)
{
	for (;;) {
		sem_wait(&tick);
		scheduler_wake_sleeping();
		scheduler_rotate();
	}
}

void time_spawn_daemons(void)
{
	if (!process_spawn(_clock_daemon, "clock daemon", HIGH, false)) {
		printf("[FAILURE]: failed to spawn clock daemon \n");
	}

	if (!process_spawn(_tick_daemon, "tick daemon", HIGH, false)) {
		printf("[FAILURE]: failed to spawn tick daemon \n");
	}
}

void time_init(void)
{
	_enable_sstc_extension();
	_enable_s_timer();
	_enable_stimecmp();

	ticks = 0;
	_update_timer();
}

void time_irq_handler(void)
{
	// Ack the timer
	_update_timer();
	ticks++;
	if (ticks % ITFREQ == 0) {
		sem_post(&clock);
	}

	sem_post(&tick);
}
