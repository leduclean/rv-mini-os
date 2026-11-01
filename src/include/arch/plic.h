/**
 * @file
 * @brief Plic configuration and dispatch.
 */

#pragma once

#include <stdint.h>

/**
 * @brief Init the plic device.
 */
void plic_init(void);

/**
 * @brief Enable a specific external device irq.
 *
 * @param irq_id The PLIC id of the peripheric.
 * @param priority The wanted priority of the irq.
 */
void plic_enable_irq(uint32_t irq_id, uint32_t priority);

/** @brief Claim the external irq, dispatch it to its device and complete it. */
void plic_handle_irq(void);
