#pragma once

#include <stdint.h>

#include <lib/clist.h>

#include <asm/asm_defs.h>

#include <drivers/virtio_blk.h>

#include <kernel/mutex.h>
#include <kernel/spinlock.h>

#define BLK_SIZE PAGE_SIZE

struct cblk {
	/* Lock protects internal metadata */
	spinlock_t lock;
	clist_node_t node;
	uint32_t blkno;
	uint32_t refcnt;
	uint8_t flags;

	/* Mutex protects access of the buffer */
	mutex_t m;
	char buf[BLK_SIZE];
};

/**
 * @brief Init the block cache.
 */
void bcache_init(void);

/**
 * @brief Read a block from the cache, on HIT it will just give a ref 
 * to the corresponding block but on MISS it will fetch to the disk.
 *
 * @param blkno The block number to read.
 */
struct cblk *bcache_read(int blkno);

/**
 * @brief Marks a block as dirty for lazy disk writing.
 *
 * @param blk A pointer to the block.
 */
void bcache_mark_dirty(struct cblk *blk);

/**
 * @brief Sync written RAM data to the disk.
 *
 * @param blk The dirty block.
 *
 * @warning This function must be called after a @p bcache_mark_dirty() call.
 * @return 0 on SUCCESS, error code < 0 if the disk request failed
 * of if the block is not dirty.
 */
int bcache_sync(struct cblk *blk);

/**
 * @brief Release the cache associated ressources.
 *
 * @param blk A pointer to the block to release.
 */
void bcache_release(struct cblk *blk);
