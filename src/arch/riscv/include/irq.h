/**
 * @file
 * @brief Machine level interrupt configuration and dispatch.
 */

#pragma once

/** @brief Disable the timer irq. */
void disable_m_timer();

/** @brief Init the timer and enable the timer irq. */
void enable_m_timer();

/** @brief Enable the machine external irq. */
void enable_m_external();

/** @brief Disable the machine external irq. */
void disable_m_external();

/** @brief Enable the supervisor external irq. */
void enable_s_external();

/** @brief Disable the supervisor external irq. */
void disable_s_external();

/**
 * @brief Config the plic for the uart irq.
 *
 * @note Enables the uart source, gives it the priority 3 and drops the
 * threshold to 0 so every non-zero priority interrupt is permitted.
 */
void plic_uart_config();

/** @brief Claim the external irq, dispatch it to its device and complete it. */
void external_irq_handler();
