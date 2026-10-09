#pragma once
#include <stdint.h>

/**
 * @brief Alloc a free block in the disk bitmap loading it in the cache.
 * @note The returned block is set to 0.
 *
 * @return blkno of the block on SUCCESS, -1 on FAILURE.
 */
int32_t balloc(void);

/**
 * @brief Free a data block and invalidate it cache entry.
 *
 * @warning ONLY data block free are allowed.
 *
 * @param blkno The block number of the data block to free.
 * @return 0 on SUCCESS, -1 if you are trying to free a non data
 * block or if the bitmap is inacessible.
 */
int bfree(uint32_t blkno);
