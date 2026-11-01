/**
 * @file
 * @brief Memory mapped io accessors.
 */

#pragma once

/**
 * @brief Access an 8 bits memory mapped register.
 * @param addr Address of the register.
 */
#define MMIO8(addr) (*(volatile uint8_t *)(uintptr_t)(addr))

/**
 * @brief Access a 16 bits memory mapped register.
 * @param addr Address of the register.
 */
#define MMIO16(addr) (*(volatile uint16_t *)(uintptr_t)(addr))

/**
 * @brief Access a 32 bits memory mapped register.
 * @param addr Address of the register.
 */
#define MMIO32(addr) (*(volatile uint32_t *)(uintptr_t)(addr))

/**
 * @brief Access a 64 bits memory mapped register.
 * @param addr Address of the register.
 */
#define MMIO64(addr) (*(volatile uint64_t *)(uintptr_t)(addr))
