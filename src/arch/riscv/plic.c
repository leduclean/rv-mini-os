#include <stdint.h>

#include <asm/board.h>
#include <asm/cpu.h>
#include <asm/csr.h>
#include <asm/mmio.h>

#include <drivers/uart.h>
#include <drivers/virtio_blk.h>

#include <kernel/time.h>

/* Register map offsets, named after the RISC-V PLIC spec "Memory Map" */
#define PLIC_PRIORITY_BASE 0x000000 /* Interrupt Priorities */
#define PLIC_PRIORITY_SIZE 4 /* One 32 bits register per source */
#define PLIC_ENABLE_BASE 0x002000 /* Interrupt Enables */
#define PLIC_ENABLE_CTX_STRIDE 0x80 /* Enable bits block per context */
#define PLIC_THRESHOLD_BASE 0x200000 /* Priority Thresholds */
#define PLIC_THRESHOLD_CTX_STRIDE 0x1000 /* Threshold block per context */
#define PLIC_CLAIM_COMPLETE_OFF 0x4 /* Claim/complete, after the threshold */

static inline uint32_t _get_context(void)
{
	//FIX: Context in S mode for hart0 is 1
	//in multihart not true
	return 1;
}

static inline uintptr_t _priority_reg(uint32_t irq_id)
{
	return PLIC_MMIO_BASE + PLIC_PRIORITY_BASE +
	       PLIC_PRIORITY_SIZE * irq_id;
}

static inline uintptr_t _enable_reg(uint32_t ctx, uint32_t irq_id)
{
	return PLIC_MMIO_BASE + PLIC_ENABLE_BASE +
	       PLIC_ENABLE_CTX_STRIDE * ctx + 4 * (irq_id / 32);
}

static inline uintptr_t _threshold_reg(uint32_t ctx)
{
	return PLIC_MMIO_BASE + PLIC_THRESHOLD_BASE +
	       PLIC_THRESHOLD_CTX_STRIDE * ctx;
}

static inline uintptr_t _claim_complete_reg(uint32_t ctx)
{
	return _threshold_reg(ctx) + PLIC_CLAIM_COMPLETE_OFF;
}

static inline void plic_enable_s_external(void)
{
	csr_set(sie, SIE_SEIE);
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
	MMIO32(_priority_reg(irq_id)) = priority;
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

	MMIO32(_threshold_reg(_get_context())) = threshold;
}

void plic_init(void)
{
	_set_priority_treshold(0);
	plic_enable_s_external();
}

void plic_enable_irq(uint32_t irq_id, uint32_t priority)
{
	MMIO32(_enable_reg(_get_context(), irq_id)) |= 1u << (irq_id % 32);
	_plic_set_pty(irq_id, priority);
}

/**
 * @brief Claim the interrupt from the plic.
 *
 * @return Plic source id of the pending interrupt.
 */
static uint32_t _claim_plic(void)
{
	return MMIO32(_claim_complete_reg(_get_context()));
}

/**
 * @brief Complete the interrupt.
 *
 * @param irq Plic source id returned by _claim_plic().
 */
static void _complete_plic(uint32_t irq)
{
	MMIO32(_claim_complete_reg(_get_context())) = irq;
}

/** @brief Claim the external irq, dispatch it to its device and complete it. */
void plic_handle_irq(void)
{
	uint32_t irq = _claim_plic();
	switch (irq) {
	case UART_PLIC_ID:
		uart_irq_handler();
		break;
	case VIRTIO_BLK_PLIC_ID:
		virtio_irq_handler();
		break;
	}
	_complete_plic(irq);
}
