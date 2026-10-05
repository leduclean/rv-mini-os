#pragma once

#define PCI_ECAM_BASE_ADDRESS 0x30000000 /* Base address of the pci bus */

/* Shifts to determine device ecam addr */
#define PCI_BUS_SHIFT 16
#define PCI_DEVICE_SHIFT 11
#define PCI_FUNC_SHIFT 8

#define PCI_VENDOR_ID 0x00 /* Vendor Id offset (word) */
#define PCI_DEV_ID 0x02 /* Device Id offset (word) */
#define PCI_CMD 0x04 /* Command offset (word) */
#define PCI_STATUS 0x06 /* Status offset (word) */
#define PCI_BAR0 0x10 /* Base Adress 0, BARX are determined with dword offset */
#define PCI_CAP_PTR 0x34 /* Capabilities pointer offset (byte) */

#define PCI_CMD_MMIO (1 << 1) /* Memory Space Enable */
#define PCI_CMD_MASTER (1 << 2) /* Bus Master Enable (DMA) */

#define PCI_BAR_FLAG_MASK (0b1111) /* Flags of a BAR, must be cleared to read */
