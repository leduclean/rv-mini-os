#include <stdint.h>

#include <lib/bitmap.h>
#include <lib/clist.h>
#include <lib/container.h>
#include <lib/string.h>

#include <drivers/virtio_blk.h>

#include <kernel/bcache.h>
#include <kernel/fs.h>
#include <kernel/mutex.h>
#include <kernel/spinlock.h>

#define CBLK_FLAGS_DIRTY \
	(1 << 0) /**< Dirty flag i.e needs sync bit indicator */
#define CBLK_FLAGS_VALID \
	(1 << 1) /**< Valid flag i.e has been fetched from the disk indicator */

#define NBUF 64
static struct cblk buf[NBUF] = { 0 };

static spinlock_t cache_lock = SPINLOCK_UNLOCKED;
static clist_node_t buf_list = CLIST_NODE_INITIALIZER(buf_list);

void bcache_init(void)
{
	struct cblk *curr;

	for (int i = 0; i < NBUF; i++) {
		curr = &buf[i];
		curr->blkno = UINT32_MAX;
		mutex_init(&curr->m);
		clist_init_node(&curr->node);
		clist_push_front(&buf_list, &curr->node);
	}
}

static inline int get_sector(uint32_t blkno)
{
	return blkno * (FS_BLK_SIZE / VIRTIO_BLK_SIZE);
}

static inline bool is_dirty_locked(struct cblk *blk)
{
	return blk->flags & CBLK_FLAGS_DIRTY;
}

static inline bool is_valid_locked(struct cblk *blk)
{
	return blk->flags & CBLK_FLAGS_VALID;
}

static inline void set_dirty_locked(struct cblk *blk)
{
	blk->flags |= CBLK_FLAGS_DIRTY;
}

static inline void clear_dirty_locked(struct cblk *blk)
{
	blk->flags &= ~CBLK_FLAGS_DIRTY;
}

static inline void set_valid_locked(struct cblk *blk)
{
	blk->flags |= CBLK_FLAGS_VALID;
}

static inline void clear_valid_locked(struct cblk *blk)
{
	blk->flags &= ~CBLK_FLAGS_VALID;
}

static int match_blkno_cb_locked(clist_node_t *node, void *args)
{
	uint32_t blkno = *(uint32_t *)args;
	struct cblk *blk = container_of(node, struct cblk, node);
	int predicate = blk->blkno == blkno;

	return predicate;
}

/**
 * @brief Performs a MRU cache request.
 */
static struct cblk *cache_request_locked(uint32_t blkno)
{

	// MRU policy
	clist_node_t *blk_node = clist_find(&buf_list, match_blkno_cb_locked,
					    &blkno);

	if (!blk_node) {
		return NULL;
	}

	return container_of(blk_node, struct cblk, node);
}

static int find_by_refcnt_cb_locked(clist_node_t *node,
				     [[maybe_unused]] void *args)
{
	struct cblk *blk = container_of(node, struct cblk, node);
	int predicate = blk->refcnt == 0 && !is_dirty_locked(blk);
	return predicate;
}

/**
 * @brief Performs a LRU scan to find reusable block
 */
static struct cblk *find_reusable_locked(void)
{
	// LRU policy
	clist_node_t *blk_node = clist_find_rev(
		&buf_list, find_by_refcnt_cb_locked, NULL);

	if (!blk_node) {
		return NULL;
	}

	return container_of(blk_node, struct cblk, node);
}

void bcache_invalidate(uint32_t blkno)
{
	spinlock_lock(&cache_lock);
	// Invalid cache entry
	struct cblk *blk = cache_request_locked(blkno);
	if (blk) {
		clear_dirty_locked(blk);
		clear_valid_locked(blk);
		blk->blkno = UINT32_MAX;
	}
	spinlock_unlock(&cache_lock);
}

struct cblk *bcache_getblk(uint32_t blkno)
{
	struct cblk *blk;
	spinlock_lock(&cache_lock);

	// HIT
	blk = cache_request_locked(blkno);
	// MISS
	if (!blk) {
		blk = find_reusable_locked();
		if (!blk) {
			panic("BUFFER OVERFLOW: buffer all used");
		}

		blk->blkno = blkno;
		clear_valid_locked(blk);
	}
	blk->refcnt++;

	spinlock_unlock(&cache_lock);
	mutex_lock(&blk->m);
	return blk;
}

struct cblk *bcache_read(uint32_t blkno)
{
	struct cblk *blk = bcache_getblk(blkno);
	// Mutex is acquire
	bool is_valid = is_valid_locked(blk);

	if (!is_valid) {
		int res = virtio_blk_request(VIRTIO_BLK_T_IN,
					     get_sector(blkno), blk->buf,
					     FS_BLK_SIZE);
		if (res < 0) {
			bcache_release(blk);
			return NULL;
		}

		set_valid_locked(blk);
	}

	return blk;
}

void bcache_mark_dirty(struct cblk *blk)
{
	set_dirty_locked(blk);
}

int bcache_sync(struct cblk *blk)
{
	int res;
	if (!is_dirty_locked(blk)) {
		return 0;
	}

	res = virtio_blk_request(VIRTIO_BLK_T_OUT, get_sector(blk->blkno),
				 blk->buf, FS_BLK_SIZE);
	if (res < 0) {
		return res;
	}
	clear_dirty_locked(blk);

	return 0;
}

void bcache_release(struct cblk *blk)
{
	mutex_unlock(&blk->m);

	spinlock_lock(&cache_lock);
	blk->refcnt--;
	// Move it to front for MRU/LRU policy
	clist_remove(&blk->node);
	clist_push_front(&buf_list, &blk->node);
	spinlock_unlock(&cache_lock);
}

//TODO: Add daemon that sync dirty released block on the
