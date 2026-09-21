/**
 * @file
 * @brief QEMU virt board memory map.
 *
 * @note Says only *where* this board wires each device. What a chip *is*
 * (register layout, bitmaps) belongs to its driver's private header.
 */

#pragma once

/* ns16550 uart */
#define UART_BASE 0x10000000

/* Platform level interrupt controller */
#define PLIC_MMIO_BASE 0x0c000000UL
#define PLIC_MMIO_SIZE 0x202000UL

/* PCIe ECAM configuration window */
#define PCI_ECAM_BASE_ADDRESS 0x30000000

/* bochs-display, mapped by _config_pcie() */
#define BOCHS_DISPLAY_BASE_ADDRESS 0x50000000

/* Video mode this board is brought up in. */
#define DISPLAY_WIDTH 1024
#define DISPLAY_HEIGHT 768
#define DISPLAY_BPP 32
#define DISPLAY_BANK 0
#define DISPLAY_SIZE (DISPLAY_WIDTH * DISPLAY_HEIGHT)
#define DISPLAY_BYTES (DISPLAY_WIDTH * DISPLAY_HEIGHT * (DISPLAY_BPP / 8))
