/**
 * @file
 * @brief Timer irq handling and system clock.
 */

#pragma once
#include "minilib/stdint.h"

/**
 * @brief Get the uptime.
 *
 * @return Number of secondes elapsed since time_init().
 */
uint32_t time_seconds();

/** @brief Reset the tick counter and arm the timer comparator. */
void time_init();

/**
 * @brief Handler for the timer irq.
 *
 * @note Advances the clock, displays it, wakes the sleeping processes,
 * reschedules and rearms the timer.
 */
void time_irq_handler(void);
