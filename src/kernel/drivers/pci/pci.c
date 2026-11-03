#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lib/err.h>
#include <lib/mmio.h>

#include <drivers/pci.h>

#include <arch/board.h>

#include "pci_regs.h"

static inline uint8_t pci_read_config8(uintptr_t dev_addr, uint32_t offset)
{
	return MMIO8(dev_addr + offset);
}

static inline uint16_t pci_read_config16(uintptr_t dev_addr, uint32_t offset)
{
	return MMIO16(dev_addr + offset);
}

static inline uint32_t pci_read_config32(uintptr_t dev_addr, uint32_t offset)
{
	return MMIO32(dev_addr + offset);
}

static inline void pci_write_config16(uintptr_t dev_addr, uint32_t offset,
				       uint16_t val)
{
	MMIO16(dev_addr + offset) = val;
}

static inline void pci_write_config32(uintptr_t dev_addr, uint32_t offset,
				       uint32_t val)
{
	MMIO32(dev_addr + offset) = val;
}

static inline uintptr_t pci_make_ecam_addr(uint32_t bus, uint32_t dev,
					    uint32_t func, uint32_t offset)
{
	return PCI_ECAM_BASE_ADDRESS | (bus << PCI_BUS_SHIFT) |
	       (dev << PCI_DEVICE_SHIFT) | (func << PCI_FUNC_SHIFT) | offset;
}

static inline bool has_cap_linked_list(uint32_t dev_ecam)
{
	uint16_t status;
	status = pci_read_config16(dev_ecam, PCI_STATUS);
	if (!(status & PCI_STATUS_CAP_LIST)) {
		return false;
	}
	return true;
}

int pci_find_device(uint16_t vendor_id, uint16_t device_id,
		    struct pci_device *out_dev)
{
	for (uint32_t dev = 0; dev <= 31; dev++) {
		uintptr_t ecam_addr = pci_make_ecam_addr(0, dev, 0, 0);

		uint16_t v_id = pci_read_config16(ecam_addr, PCI_VENDOR_ID);

		// Fast path: no dev
		if (v_id == 0xFFFF) {
			continue;
		}

		uint16_t d_id = pci_read_config16(ecam_addr, PCI_DEV_ID);
		if (v_id == vendor_id && d_id == device_id) {
			out_dev->ecam_addr = ecam_addr;
			out_dev->cap_ptr = 0;
			if (has_cap_linked_list(ecam_addr)) {
				out_dev->cap_ptr = pci_read_config8(
							   ecam_addr,
							   PCI_CAP_PTR) &
						   ~PCI_CAP_RESERVED;
			}
			return 0;
		}
	}

	return -ENODEV;
}

void pci_enable_device(struct pci_device *dev)
{
	uint32_t ecam_addr = dev->ecam_addr;
	uint16_t cmd = pci_read_config16(ecam_addr, PCI_CMD);
	pci_write_config16(ecam_addr, PCI_CMD, cmd | PCI_CMD_MMIO);
}

void pci_set_master(struct pci_device *dev)
{
	uint32_t ecam_addr = dev->ecam_addr;
	uint16_t cmd = pci_read_config16(ecam_addr, PCI_CMD);
	pci_write_config16(ecam_addr, PCI_CMD, cmd | PCI_CMD_MASTER);
}

static inline uint32_t pci_bar_reg(uint8_t n)
{
	return PCI_BAR0 + sizeof(uint32_t) * n;
}

static inline bool pci_bar_is_64(uint32_t bar)
{
	return (bar & PCI_BAR_TYPE_MASK) == PCI_BAR_TYPE_64;
}

int64_t pci_get_bar_size(struct pci_device *dev, uint8_t n)
{
	if (n >= PCI_BAR_COUNT) {
		return -EINVAL;
	}

	uint32_t ecam_addr = dev->ecam_addr;
	uint32_t reg = pci_bar_reg(n);
	uint32_t bar = pci_read_config32(ecam_addr, reg);

	if (bar & PCI_BAR_IO) {
		return -EIO;
	}

	pci_write_config32(ecam_addr, reg, 0xFFFFFFFF);
	uint64_t mask = pci_read_config32(ecam_addr, reg) & ~PCI_BAR_FLAG_MASK;
	if (mask == 0) {
		pci_write_config32(ecam_addr, reg, bar);
		// BAR not unimplemented
		return -EINVAL;
	}
	pci_write_config32(ecam_addr, reg, bar);

	if (pci_bar_is_64(bar)) {
		if (n == PCI_BAR_COUNT - 1) {
			// Need at least two bar for 64 bits bar.
			return -EINVAL;
		}

		uint32_t reg_hi = pci_bar_reg(n + 1);
		uint32_t bar_hi = pci_read_config32(ecam_addr, reg_hi);

		pci_write_config32(ecam_addr, reg_hi, 0xFFFFFFFF);
		uint32_t hi_m = pci_read_config32(ecam_addr, reg_hi);
		pci_write_config32(ecam_addr, reg_hi, bar_hi);

		mask |= ((uint64_t)hi_m << 32);
	} else {
		mask |= ((uint64_t)UINT32_MAX << 32);
	}

	return ~mask + 1;
}

int pci_set_bar(struct pci_device *dev, uint8_t n, uint64_t addr)
{
	if (n >= PCI_BAR_COUNT) {
		return -EINVAL;
	}


	uint32_t ecam_addr = dev->ecam_addr;
	uint32_t reg = pci_bar_reg(n);
	uint32_t bar = pci_read_config32(ecam_addr, reg);

	if (bar & PCI_BAR_IO) {
		return -EIO;
	}

	pci_write_config32(ecam_addr, reg, (uint32_t)addr);

	if (pci_bar_is_64(bar)) {
		if (n == PCI_BAR_COUNT - 1) {
			// Need at least two bar for 64 bits bar.
			return -EINVAL;
		}

		pci_write_config32(ecam_addr, pci_bar_reg(n + 1),
				    (uint32_t)(addr >> 32));
	}

	return 0;
}

int pci_bar_addr(const struct pci_device *dev, uint8_t n, uint64_t *addr)
{
	if (n >= PCI_BAR_COUNT) {
		return -EINVAL;
	}

	uint32_t bar = pci_read_config32(dev->ecam_addr, pci_bar_reg(n));
	if (bar & PCI_BAR_IO) {
		return -EIO;
	}

	uint64_t res = bar & ~PCI_BAR_FLAG_MASK;
	if (pci_bar_is_64(bar)) {
		if (n == PCI_BAR_COUNT - 1) {
			return -EINVAL;
		}
		res |= (uint64_t)pci_read_config32(dev->ecam_addr,
						    pci_bar_reg(n + 1))
		       << 32;
	}
	*addr = res;

	return 0;
}

struct pci_cap *pci_find_cap(const struct pci_device *dev,
			     const struct pci_cap *prev, uint8_t cap_id)
{
	uint8_t cap_off = prev ? prev->cap_next & ~PCI_CAP_RESERVED
			       : dev->cap_ptr;

	while (cap_off != 0) {
		struct pci_cap *curr =
			(struct pci_cap *)((uintptr_t)dev->ecam_addr + cap_off);
		if (curr->cap_vndr == cap_id) {
			return curr;
		}
		cap_off = curr->cap_next & ~PCI_CAP_RESERVED;
	}

	return NULL;
}
