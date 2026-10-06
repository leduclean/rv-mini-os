#pragma once

// Shift definition for PCI config
#include <stdint.h>

struct pci_device {
	uint32_t ecam_addr; /* Device ecam addr */
	uint8_t cap_ptr; /* First capability offset, 0 if none */
};

struct pci_cap {
	uint8_t cap_vndr; /* Cap Vendor */
	uint8_t cap_next; /* Cap next pointer */
	uint8_t cap_len; /* Cap len of the structure */
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
 * @brief Assign a physical address to a memory BAR of the device.
 *
 * Must be called before enabling the device pci_enable_device().
 *
 * @param dev Device descriptor.
 * @param n BAR number (0 to 5).
 * @param addr Physical address, MUST be aligned on the BAR size.
 * @return size of the bar adress space on success, -1 if n is invalid, the BAR is I/O or unimplemented,
 * or addr is misaligned.
 */
int32_t pci_set_bar(struct pci_device *dev, uint8_t n, uint64_t addr);

/**
 * @brief Read the physical address assigned to a memory BAR.
 *
 * @param dev Device descriptor.
 * @param n BAR number (0 to 5).
 * @param addr[out] The BAR physical address.
 * @return 0 on success, -1 if n is invalid or the BAR is I/O.
 */
int pci_bar_addr(const struct pci_device *dev, uint8_t n, uint64_t *addr);

/**
 * @brief Iterate over the device capabilities matching @p cap_id.
 *
 * @param dev Device descriptor.
 * @param prev Previously returned cap, or NULL to start from the first one.
 * @param cap_id The target cap_vndr.
 * @return The next matching cap, or NULL if none.
 */
struct pci_cap *pci_find_cap(const struct pci_device *dev,
			     const struct pci_cap *prev, uint8_t cap_id);
