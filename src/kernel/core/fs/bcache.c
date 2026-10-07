#include <stdint.h>

#include <lib/clist.h>
#include <lib/container.h>

#include <drivers/virtio_blk.h>

#include <kernel/bcache.h>
#include <kernel/mutex.h>
#include <kernel/spinlock.h>

#define CBLK_FLAGS_DIRTY \
	(1 << 0) /**< Dirty flag i.e needs sync bit indicator */
#define CBLK_FLAGS_VALID \
	(1 << 1) /**< Valid flag i.e has been fetched from the disk indicator */

#define NBUF 64
static struct cblk buf[NBUF] = { 0 };

static spinlock_t list_lock = SPINLOCK_UNLOCKED;
static clist_node_t buf_list = CLIST_NODE_INITIALIZER(buf_list);

void bcache_init(void)
{
	struct cblk *curr;

	for (int i = 0; i < NBUF; i++) {
		curr = &buf[i];
		spinlock_init(&curr->lock);
		mutex_init(&curr->m);
		clist_init_node(&curr->node);
		clist_push_front(&buf_list, &curr->node);
	}
}

static inline int _get_sector(int blkno)
{
	return blkno * (BLK_SIZE / VIRTIO_BLK_SIZE);
}

static inline bool _is_dirty_locked(struct cblk *blk)
{
	return blk->flags & CBLK_FLAGS_DIRTY;
}

static inline bool _is_valid_locked(struct cblk *blk)
{
	return blk->flags & CBLK_FLAGS_VALID;
}

static inline void _set_dirty_locked(struct cblk *blk)
{
	blk->flags |= CBLK_FLAGS_DIRTY;
}

static inline void _clear_dirty_locked(struct cblk *blk)
{
	blk->flags &= ~CBLK_FLAGS_DIRTY;
}

static inline void _set_valid_locked(struct cblk *blk)
{
	blk->flags |= CBLK_FLAGS_VALID;
}

static inline void _clear_valid_locked(struct cblk *blk)
{
	blk->flags &= ~CBLK_FLAGS_VALID;
}

static int _match_blkno_cb(clist_node_t *node, void *args)
{
	uint32_t blkno = *(uint32_t *)args;
	struct cblk *blk = container_of(node, struct cblk, node);

	spinlock_lock(&blk->lock);

	int predicate = blk->blkno == blkno;

	spinlock_unlock(&blk->lock);

	return predicate;
}

/**
 * @brief Performs a MRU cache request.
 */
static struct cblk *_cache_request_locked(int blkno)
{

	// MRU policy
	clist_node_t *blk_node = clist_find(&buf_list, _match_blkno_cb, &blkno);

	if (!blk_node) {
		return NULL;
	}

	return container_of(blk_node, struct cblk, node);
}

static int _find_by_refcnt_cb(clist_node_t *node, [[maybe_unused]] void *args)
{
	struct cblk *blk = container_of(node, struct cblk, node);

	spinlock_lock(&blk->lock);

	int predicate = blk->refcnt == 0 && !_is_dirty_locked(blk);

	spinlock_unlock(&blk->lock);

	return predicate;
}

/**
 * @brief Performs a LRU scan to find reusable block
 */
static struct cblk *_find_reusable_locked(void)
{
	// LRU policy
	clist_node_t *blk_node = clist_find_rev(&buf_list, _find_by_refcnt_cb,
						NULL);

	if (!blk_node) {
		return NULL;
	}

	return container_of(blk_node, struct cblk, node);
}

static struct cblk *_get_block(int blkno)
{
	struct cblk *blk;
	spinlock_lock(&list_lock);

	// HIT
	blk = _cache_request_locked(blkno);

	// MISS
	if (!blk) {
		blk = _find_reusable_locked();
		if (!blk) {
			panic("BUFFER OVERFLOW: buffer all used");
		}

		spinlock_lock(&blk->lock);
		blk->blkno = blkno;
		_clear_valid_locked(blk);
		spinlock_unlock(&blk->lock);
	};
	spinlock_lock(&blk->lock);
	blk->refcnt++;
	spinlock_unlock(&blk->lock);

	spinlock_unlock(&list_lock);
	mutex_lock(&blk->m);
	return blk;
}

struct cblk *bcache_read(int blkno)
{
	struct cblk *blk = _get_block(blkno);

	spinlock_lock(&blk->lock);
	bool is_valid = _is_valid_locked(blk);
	spinlock_unlock(&blk->lock);

	if (!is_valid) {
		int res = virtio_blk_request(VIRTIO_BLK_T_IN,
					     _get_sector(blkno), blk->buf,
					     BLK_SIZE);
		if (res < 0) {
			bcache_release(blk);
			return NULL;
		}

		spinlock_lock(&blk->lock);
		_set_valid_locked(blk);
		spinlock_unlock(&blk->lock);
	}

	return blk;
}

void bcache_mark_dirty(struct cblk *blk)
{
	spinlock_lock(&blk->lock);
	_set_dirty_locked(blk);
	spinlock_unlock(&blk->lock);
}

int bcache_sync(struct cblk *blk)
{
	int res;

	spinlock_lock(&blk->lock);
	if (!_is_dirty_locked(blk)) {
		spinlock_unlock(&blk->lock);
		return 0;
	}

	spinlock_unlock(&blk->lock);

	res = virtio_blk_request(VIRTIO_BLK_T_OUT, _get_sector(blk->blkno),
				 blk->buf, BLK_SIZE);
	if (res < 0) {
		return res;
	}

	spinlock_lock(&blk->lock);
	_clear_dirty_locked(blk);
	spinlock_unlock(&blk->lock);

	return 0;
}

void bcache_release(struct cblk *blk)
{
	spinlock_lock(&list_lock);
	spinlock_lock(&blk->lock);
	blk->refcnt--;

	// Move it to front for MRU/LRU policy
	clist_remove(&blk->node);
	clist_push_front(&buf_list, &blk->node);

	spinlock_unlock(&blk->lock);
	spinlock_unlock(&list_lock);
	mutex_unlock(&blk->m);
}
