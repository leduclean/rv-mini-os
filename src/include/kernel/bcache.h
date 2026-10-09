#pragma once

#include <stdint.h>

#include <lib/clist.h>

#include <kernel/fs.h>
#include <kernel/mutex.h>

struct cblk {
	/* The folowwing fields are ptotected by 
     * the cache lock itself */
	clist_node_t node;
	uint32_t blkno;
	uint32_t refcnt;

	/* Mutex protects access of the buffer content
     * and the relative flags*/
	mutex_t m;
	uint8_t flags;
	char buf[FS_BLK_SIZE];
};

/**
 * @brief Init the block cache.
 */
void bcache_init(void);

/**
 * @brief Alloc a free block in the disk bitmap loading it in the cache.
 * @note The returned block is set to 0.
 *
 * @return blkno of the block on SUCCESS, -1 on FAILURE.
 */
int32_t bcache_alloc(void);

/**
 * @brief Free a data block and invalidate it cache entry.
 *
 * @warning ONLY data block free are allowed.
 *
 * @param blkno The block number of the data block to free.
 * @return 0 on SUCCESS, -1 if you are trying to free a non data
 * block or if the bitmap is inacessible.
 */
int bcache_free(uint32_t blkno);

/**
 * @brief Gets a block by block number. The block might be already used 
 * if in cache or it might be incoherent to the disk if it's just a buffer.
 *
 * @param blkno The block number
 *
 * @post Returned block is locked; release it with bcache_release().
 */
struct cblk *bcache_getblk(uint32_t blkno);
/**
 * @brief Read a block from the cache, on HIT it will just give a ref 
 * to the corresponding block but on MISS it will fetch to the disk.
 *
 * @param blkno The block number to read.
 */
struct cblk *bcache_read(uint32_t blkno);

/**
 * @brief Marks a block as dirty for lazy disk writing.
 *
 * @param blk A pointer to the block.
 *
 * @pre  @p blk obtained by bcache_getblk()/bcache_read(), not yet released.*
 */
void bcache_mark_dirty(struct cblk *blk);

/**
 * @brief Sync written RAM data to the disk.
 *
 * @param blk The dirty block.
 *
 * @note Calling this functino on a non dirty block will just led to unchanged state.
 * @return 0 on SUCCESS, error code < 0 if the disk request failed.
 * @pre @p blk obtained by bcache_getblk()/bcache_read(), not yet released.*
 */
int bcache_sync(struct cblk *blk);

/**
 * @brief Release the cache associated ressources.
 *
 * @param blk A pointer to the block to release.
 * @pre @p blk obtained by bcache_getblk()/bcache_read(), not yet released.
 * @post @p blk must not be used anymore.ase.
 */
void bcache_release(struct cblk *blk);
