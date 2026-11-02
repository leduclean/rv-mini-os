#pragma once
#include <stdint.h>

/**
 * @brief Alloc a free block in the disk bitmap loading it in the cache.
 * @note The returned block is set to 0.
 *
 * @return The block number.
 * @retval -ENOSPC Disk full.
 * @retval -EIO Disk failure.
 */
int32_t balloc(void);

/**
 * @brief Free a data block and invalidate it cache entry.
 *
 * @warning ONLY data block free are allowed.
 *
 * @param blkno The block number of the data block to free.
 * @retval 0 Success.
 * @retval -EINVAL @p blkno is not a data block.
 * @retval -EIO Disk failure.


 */
int bfree(uint32_t blkno);
