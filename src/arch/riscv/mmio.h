#pragma once

// Macro definition for memory mapped container
#define MMIO8(addr) (*(volatile uint8_t *)(uintptr_t)(addr))
#define MMIO16(addr) (*(volatile uint16_t *)(uintptr_t)(addr))
#define MMIO32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))
#define MMIO64(addr) (*(volatile uint64_t *)(uintptr_t)(addr))
