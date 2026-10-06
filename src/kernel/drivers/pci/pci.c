#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <asm/mmio.h>

#include <drivers/pci.h>

#include "pci_regs.h"

static inline uint8_t _pci_read_config8(uintptr_t dev_addr, uint32_t offset)
{
	return MMIO8(dev_addr + offset);
}

static inline uint16_t _pci_read_config16(uintptr_t dev_addr, uint32_t offset)
{
	return MMIO16(dev_addr + offset);
}

static inline uint32_t _pci_read_config32(uintptr_t dev_addr, uint32_t offset)
{
	return MMIO32(dev_addr + offset);
}

static inline void _pci_write_config16(uintptr_t dev_addr, uint32_t offset,
				       uint16_t val)
{
	MMIO16(dev_addr + offset) = val;
}

static inline void _pci_write_config32(uintptr_t dev_addr, uint32_t offset,
				       uint32_t val)
{
	MMIO32(dev_addr + offset) = val;
}

static inline uintptr_t _pci_make_ecam_addr(uint32_t bus, uint32_t dev,
					    uint32_t func, uint32_t offset)
{
	return PCI_ECAM_BASE_ADDRESS | (bus << PCI_BUS_SHIFT) |
	       (dev << PCI_DEVICE_SHIFT) | (func << PCI_FUNC_SHIFT) | offset;
}

static inline bool _has_cap_linked_list(uint32_t dev_ecam)
{
	uint16_t status;
	status = _pci_read_config16(dev_ecam, PCI_STATUS);
	if (!(status & PCI_STATUS_CAP_LIST)) {
		return false;
	}
	return true;
}

bool pci_find_device(uint16_t vendor_id, uint16_t device_id,
		     struct pci_device *out_dev)
{
	for (uint32_t dev = 0; dev <= 31; dev++) {
		uintptr_t ecam_addr = _pci_make_ecam_addr(0, dev, 0, 0);

		uint16_t v_id = _pci_read_config16(ecam_addr, PCI_VENDOR_ID);

		// Fast path: no dev
		if (v_id == 0xFFFF) {
			continue;
		}

		uint16_t d_id = _pci_read_config16(ecam_addr, PCI_DEV_ID);
		if (v_id == vendor_id && d_id == device_id) {
			out_dev->ecam_addr = ecam_addr;
			out_dev->cap_ptr = 0;
			if (_has_cap_linked_list(ecam_addr)) {
				out_dev->cap_ptr = _pci_read_config8(
							   ecam_addr,
							   PCI_CAP_PTR) &
						   ~PCI_CAP_RESERVED;
			}
			return true;
		}
	}

	return false;
}

void pci_enable_device(struct pci_device *dev)
{
	uint32_t ecam_addr = dev->ecam_addr;
	uint16_t cmd = _pci_read_config16(ecam_addr, PCI_CMD);
	_pci_write_config16(ecam_addr, PCI_CMD, cmd | PCI_CMD_MMIO);
}

void pci_set_master(struct pci_device *dev)
{
	uint32_t ecam_addr = dev->ecam_addr;
	uint16_t cmd = _pci_read_config16(ecam_addr, PCI_CMD);
	_pci_write_config16(ecam_addr, PCI_CMD, cmd | PCI_CMD_MASTER);
}

static inline uint32_t _pci_bar_reg(uint8_t n)
{
	return PCI_BAR0 + sizeof(uint32_t) * n;
}

static inline bool _pci_bar_is_64(uint32_t bar)
{
	return (bar & PCI_BAR_TYPE_MASK) == PCI_BAR_TYPE_64;
}

int32_t pci_set_bar(struct pci_device *dev, uint8_t n, uint64_t addr)
{
	if (n >= PCI_BAR_COUNT) {
		return -1;
	}

	uint32_t ecam_addr = dev->ecam_addr;
	uint32_t reg = _pci_bar_reg(n);
	uint32_t bar = _pci_read_config32(ecam_addr, reg);

	if (bar & PCI_BAR_IO) {
		return -1;
	}

	bool is_64 = _pci_bar_is_64(bar);
	if (is_64 && n == PCI_BAR_COUNT - 1) {
		// Need at least two bar for 64 bits bar.
		return -1;
	}

	// This is the common method to determine the address space needed by the PCI.
	_pci_write_config32(ecam_addr, reg, 0xFFFFFFFF);
	uint32_t align_mask = _pci_read_config32(ecam_addr, reg) &
			      ~PCI_BAR_FLAG_MASK;
	uint32_t size = ~align_mask + 1;
	_pci_write_config32(ecam_addr, reg, bar);

	// Adress is misaligned.
	if (addr & align_mask) {
		return -1;
	}

	_pci_write_config32(ecam_addr, reg, (uint32_t)addr);
	if (is_64) {
		_pci_write_config32(ecam_addr, _pci_bar_reg(n + 1),
				    (uint32_t)(addr >> 32));
	}

	return size;
}

int pci_bar_addr(const struct pci_device *dev, uint8_t n, uint64_t *addr)
{
	if (n >= PCI_BAR_COUNT) {
		return -1;
	}

	uint32_t bar = _pci_read_config32(dev->ecam_addr, _pci_bar_reg(n));
	if (bar & PCI_BAR_IO) {
		return -1;
	}

	uint64_t res = bar & ~PCI_BAR_FLAG_MASK;
	if (_pci_bar_is_64(bar)) {
		if (n == PCI_BAR_COUNT - 1) {
			return -1;
		}
		res |= (uint64_t)_pci_read_config32(dev->ecam_addr,
						    _pci_bar_reg(n + 1))
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
