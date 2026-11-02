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
 * @brief Invalidate a cache entry, this is like a tlb invalidation.
 *
 * @param blkno Block id of the block to invalidate. 
 */
void bcache_invalidate(uint32_t blkno);

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
 * @return The locked block.
 * @retval NULL Disk failure, callers report -EIO.
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
 * @retval 0 Success.
 * @retval -Exxx See virtio_blk_request().


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
