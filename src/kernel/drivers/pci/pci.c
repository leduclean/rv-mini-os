#include <stdbool.h>
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
			out_dev->cap_ptr = _pci_read_config8(ecam_addr,
							     PCI_CAP_PTR);
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

int pci_set_bar(struct pci_device *dev, uint8_t n, uint32_t addr)
{
	if (n >= 6) {
		return -1;
	}

	uint32_t ecam_addr = dev->ecam_addr;
	// Clear bar bits
	addr &= ~PCI_BAR_FLAG_MASK;
	_pci_write_config32(ecam_addr, PCI_BAR0 + sizeof(uint32_t) * n, addr);

	return 0;
}
