#include <limits.h>
#include <stdint.h>

#include <lib/bitmap.h>

int bm_find_first_zero(const void *buf, uint32_t bit_size)
{
	uint64_t *bitmap64 = (uint64_t *)buf;
	uint8_t bit_in_64 = sizeof(uint64_t) * CHAR_BIT;
	uint32_t max_qw = bit_size / bit_in_64;

	for (uint32_t qw = 0; qw < max_qw; qw++) {
		uint64_t qword = bitmap64[qw];
		if (qword == UINT64_MAX) {
			// All bit at 1
			continue;
		}
		int bit_idx = __builtin_ctzll(~qword);
		return bit_idx + qw * bit_in_64;
	}

	return -1;
};

void bm_set_bit(void *buf, int bit_idx)
{
	uint8_t *bitmap = (uint8_t *)buf;
	int idx = bit_idx / (sizeof(uint8_t) * CHAR_BIT);
	int byte_shift = bit_idx % (sizeof(uint8_t) * CHAR_BIT);
	bitmap[idx] |= (1 << byte_shift);
}

void bm_clear_bit(void *buf, int bit_idx)
{
	uint8_t *bitmap = (uint8_t *)buf;
	int idx = bit_idx / (sizeof(uint8_t) * CHAR_BIT);
	int byte_shift = bit_idx % (sizeof(uint8_t) * CHAR_BIT);
	bitmap[idx] &= ~(1 << byte_shift);
}
