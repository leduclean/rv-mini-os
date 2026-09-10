#pragma once

// Memory Address
#define TA_HEAP_BLOCK 256
#define TA_BLOCK_SPLIT 256
#define TA_ALIGNMENT 8

// PMPaddr bit values
#define PMP_R_BIT (1 << 0)
#define PMP_W_BIT (1 << 1)
#define PMP_X_BIT (1 << 2)
#define PMP_A_NAPOT (3 << 3)

/**
 * @brief Stub memory management waiting for the supervisor and paging policy
 * TODO: Remove me when supervisor and paging are supported.
 */
void pmp_allow_all();
