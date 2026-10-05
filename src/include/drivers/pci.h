#pragma once

// Shift definition for PCI config
#include <stdint.h>

struct pci_device {
	uint32_t ecam_addr; /* Device ecam addr */
	uint8_t cap_ptr; /* Capabilities ptr */
};

/**
 * @brief Find a pci device and fill it corresponding @p struct pci_device.
 *
 * @param vendor_id Vendor id of the device.
 * @param device_id Device id. 
 * @param out_dev The structure to fill.
 * @return True if finded else false.
 */
bool pci_find_device(uint16_t vendor_id, uint16_t device_id,
		     struct pci_device *out_dev);

/**
 * @brief Enable the device MMIO.
 *
 * @param dev Device descriptor.
 */
void pci_enable_device(struct pci_device *dev);

/**
 * @brief Enable the device master config via DMA.
 *
 * @param dev Device descriptor.
 */
void pci_set_master(struct pci_device *dev);

/**
 * @brief Set a specific BAR of the device.
 *
 * @warning n MUST be between 0 and 5.
 *
 * @param dev Device descriptor.
 * @param n BAR number.
 * @param addr The value of the addr.
 * @return 0 if n is between 0 and 5 else -1.
 */
int pci_set_bar(struct pci_device *dev, uint8_t n, uint32_t addr);
