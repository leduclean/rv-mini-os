/**
 * @file
 * @brief Plic configuration and dispatch.
 */

#pragma once

/** @brief Enable the supervisor external irq. */
void plic_enable_s_external();

/** @brief Disable the supervisor external irq. */
void plic_disable_s_external();

/**
 * @brief Config the plic for the uart irq.
 *
 * @note Enables the uart source, gives it the priority 3 and drops the
 * threshold to 0 so every non-zero priority interrupt is permitted.
 */
void plic_config_uart();

/** @brief Claim the external irq, dispatch it to its device and complete it. */
void plic_handle_irq();
