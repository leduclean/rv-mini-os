#pragma once

/**
 * @brief Alloc a free block in the disk.
 *
 * @return blkno of the block on SUCCESS, -1 on FAILURE.
 */
int balloc(void);

/**
 * @brief Free a data block.
 *
 * @warning ONLY data block free are allowed.
 * @param blkno The block number of the data block to free.
 * @return 0 on SUCCESS, -1 if you are trying to free a non data
 * block or if the bitmap is inacessible.
 */
int bfree(int blkno);
