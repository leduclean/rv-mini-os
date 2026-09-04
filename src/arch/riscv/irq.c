#include "csr.h"
#include "mmio.h"
#include "platform.h"
#include "time.h"
#include "uart.h"

void enable_s_external()
{
	csr_set(sie, (1 << S_IRQ_EXT));
}

void disable_s_external()
{
	csr_clear(sie, (1 << S_IRQ_EXT));
}

/**
 * @brief Set the priority of a specific irq device.
 *
 * @note The range of priority is between 0 and 7, 0 meaning never interrupt.
 * A higher value is clamped to 7.
 *
 * @param irq_id Plic source id of the device.
 * @param priority Priority to set.
 */
static void _plic_set_pty(uint32_t irq_id, uint32_t priority)
{
	if (priority > 7) {
		// range is between 0 and 7.
		priority = 7;
	}
	MMIO32(PLIC_SOURCE + (irq_id << 2)) = priority;
}

/**
 * @brief Set the priority threshold of the plic target.
 *
 * @note Supports 7 levels of priority: a threshold value of zero permits all
 * interrupts with a non-zero priority, whereas a value of 7 masks all
 * interrupts. A higher value is clamped to 7.
 *
 * @param threshold Threshold to set.
 */
static void _set_priority_treshold(uint32_t threshold)
{
	if (threshold > 7) {
		threshold = 7;
	}
	MMIO32(PLIC_TARGET_S) = threshold;
}

void plic_uart_config()
{
	/* Enable UART irq */
	MMIO32(PLIC_ENABLE_S) |= PLIC_ENABLE_UART;
	/* Set Uart priority to  3 */
	_plic_set_pty(PLIC_UART_ID, 3);
	/* Set threshold to 0 to enable all < 0 interrupts */
	_set_priority_treshold(0);
}

/**
 * @brief Claim the interrupt from the plic.
 *
 * @return Plic source id of the pending interrupt.
 */
static uint32_t _claim_plic()
{
	return MMIO32(PLIC_IRQ_CLAIM_S);
}

/**
 * @brief Complete the interrupt.
 *
 * @param irq Plic source id returned by _claim_plic().
 */
static void _complete_plic(uint32_t irq)
{
	MMIO32(PLIC_IRQ_CLAIM_S) = irq;
}

/** @brief Claim the external irq, dispatch it to its device and complete it. */
void external_irq_handler()
{
	uint32_t irq = _claim_plic();
	switch (irq) {
	case PLIC_UART_ID:
		uart_irq_handler();
		break;
	}
	_complete_plic(irq);
}
