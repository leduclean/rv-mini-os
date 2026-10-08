#pragma once
#include <stdint.h>

/**
 * @brief Find the first zero in a bitmap buffer.
 *
 * @param buf Pointer to bitmap buffer.
 * @param bit_size The size of the buffer in bit.
 * @return Bit idx of the first zero.
 */
int bm_find_first_zero(const void *buf, uint32_t bit_size);

/**
 * @brief Sets a bit in a bitmap buffer.
 *
 * @param buf Pointer to bitmap buffer.
 * @param bit_idx idx of the bit to set.
 */
void bm_set_bit(void *buf, int bit_idx);

/**
 * @brief Clears a bit in a bitmap buffer.
 *
 * @param buf Pointer to bitmap buffer.
 * @param bit_idx idx of the bit to clear.
 */
void bm_clear_bit(void *buf, int bit_idx);
