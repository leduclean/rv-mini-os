/**
 * @file
 * @brief Timer irq handling and system clock.
 */

#pragma once
#include "minilib/stdint.h"

/**
 * @brief Get the uptime.
 *
 * @return Number of secondes elapsed since init_timer().
 */
uint32_t seconds();

/** @brief Reset the tick counter and arm the timer comparator. */
void init_timer();

/**
 * @brief Handler for the timer irq.
 *
 * @note Advances the clock, displays it, wakes the sleeping processes,
 * reschedules and rearms the timer.
 */
void timer_irq_handler(void);
