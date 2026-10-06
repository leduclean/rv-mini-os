#pragma once

/* Shifts to determine device ecam addr */
#define PCI_BUS_SHIFT 20
#define PCI_DEVICE_SHIFT 15
#define PCI_FUNC_SHIFT 12

#define PCI_VENDOR_ID 0x00 /* Vendor Id offset (word) */
#define PCI_DEV_ID 0x02 /* Device Id offset (word) */
#define PCI_CMD 0x04 /* Command offset (word) */
#define PCI_STATUS 0x06 /* Status offset (word) */
#define PCI_BAR0 0x10 /* Base Adress 0, BARX are determined with dword offset */
#define PCI_CAP_PTR 0x34 /* Capabilities pointer offset (byte) */

#define PCI_CMD_MMIO (1 << 1) /* Memory Space Enable */
#define PCI_CMD_MASTER (1 << 2) /* Bus Master Enable (DMA) */

#define PCI_STATUS_CAP_LIST \
	(1 << 4) /* Capabilitie list,set to 1: 0x34 is a linked list of cap */

#define PCI_BAR_COUNT 6 /* Number of BARs of a type 0 header */
#define PCI_BAR_FLAG_MASK 0b1111 /* Flags of a BAR, must be cleared to read */
#define PCI_BAR_IO (1 << 0) /* Set if the BAR maps I/O space, not memory */
#define PCI_BAR_TYPE_MASK 0b110 /* Memory BAR type */
#define PCI_BAR_TYPE_64 0b100 /* 64-bit BAR, upper half in the next BAR */

#define PCI_CAP_RESERVED \
	0b11 /* Reserved bit of the cap, must be cleared when reading */
