/**
 * @file
 * @brief Machine level interrupt configuration and dispatch.
 */

#pragma once
#include "minilib/stdint.h"

/**
 * @brief Set the trap entry point called to treat the irq.
 *
 * @param entry Trap vector written in the mtvec register.
 */
void init_trap_entry(void (*entry)());

/**
 * @brief Trap handler triggered by the entry point.
 *
 * @note Dispatches to the external or to the timer handler, depending on
 * the cause and on the enabled interrupts.
 *
 * @param mcause Trap cause, its bit 63 is ignored.
 * @param mie Enabled interrupts.
 * @param mip Pending interrupts.
 */
void trap_handler(uint64_t mcause, uint64_t mie, uint64_t mip);

/** @brief Disable the timer irq. */
void disable_timer();

/** @brief Init the timer and enable the timer irq. */
void enable_timer();

/** @brief Enable the machine external irq. */
void enable_external();

/** @brief Disable the machine external irq. */
void disable_external();

/**
 * @brief Config the plic for the uart irq.
 *
 * @note Enables the uart source, gives it the priority 3 and drops the
 * threshold to 0 so every non-zero priority interrupt is permitted.
 */
void plic_uart_config();
