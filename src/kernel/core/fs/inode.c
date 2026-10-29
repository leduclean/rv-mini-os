/*
 * Locking:
 *   cache_lock: refcnt, id, node.
 *   inode->m:   flags, content (while refcnt > 0).
 *
 * A slot with refcnt == 0 belongs to the cache: cache_lock alone is enough.
 * cache_lock and inode->m are never held together.
 * inode_put() must be called without inode->m.
 */

#include <stddef.h>
#include <stdint.h>

#include <lib/bitmap.h>
#include <lib/string.h>

#include <drivers/virtio_blk.h>

#include <kernel/balloc.h>
#include <kernel/bcache.h>
#include <kernel/fs.h>
#include <kernel/inode.h>
#include <kernel/mutex.h>

#include "kernel/vpages.h"
#include "lib/types.h"

#define INODE_CACHE_SIZE 64
#define INODES_PER_BLK (FS_BLK_SIZE / sizeof(struct dinode))
#define INODE_NB (FS_NB_INODE * INODES_PER_BLK)

static spinlock_t cache_lock = SPINLOCK_UNLOCKED;
static struct inode islots[INODE_CACHE_SIZE];
static clist_node_t icache = CLIST_NODE_INITIALIZER(icache);

static inline int _get_blk(uint32_t id)
{
	return FS_INODE_TABLE_START + id / INODES_PER_BLK;
}
static inline int _get_tab_idx(uint32_t id)
{
	return id % INODES_PER_BLK;
}

static int _reserve_first_inode()
{

	uint32_t blkno = FS_INODE_BM_START;
	struct cblk *blk = bcache_read(blkno);
	if (!blk) {
		return -1;
	}
	bm_set_bit(blk->buf, 0);
	bcache_mark_dirty(blk);
	bcache_release(blk);
	return 0;
};

int inode_init(void)
{
	int res = _reserve_first_inode();
	if (res < 0) {
		return res;
	}

	struct inode *curr;
	for (int i = 0; i < INODE_CACHE_SIZE; i++) {
		curr = &islots[i];
		curr->id = UINT32_MAX;
		mutex_init(&curr->m);
		clist_init_node(&curr->node);
		clist_push_front(&icache, &curr->node);
	}

	return 0;
}

static int32_t _ibm_alloc(void)
{
	uint32_t nb_ibm = fs_get_nb_ibm();
	uint32_t bm_bit_per_blk = fs_get_bm_bits_per_blk();

	for (uint32_t i = 0; i < nb_ibm; i++) {
		uint32_t bm_blk_no = FS_INODE_BM_START + i;

		struct cblk *bm_blk = bcache_read(bm_blk_no);
		if (!bm_blk) {
			return -1;
		}

		int bit_idx = bm_find_first_zero(bm_blk->buf, bm_bit_per_blk);
		if (bit_idx >= 0) {
			bm_set_bit(bm_blk->buf, bit_idx);
			bcache_mark_dirty(bm_blk);
			bcache_release(bm_blk);
			return bit_idx + (i * bm_bit_per_blk);
		}

		bcache_release(bm_blk);
	}

	return -1;
}

static int32_t _ibm_free(uint32_t id)
{
	if (id >= INODE_NB) {
		return -1;
	}

	uint32_t bm_bit_per_blk = fs_get_bm_bits_per_blk();
	uint32_t bm_blk_no = FS_INODE_BM_START + id / bm_bit_per_blk;

	struct cblk *bm_blk = bcache_read(bm_blk_no);
	if (!bm_blk) {
		return -1;
	}

	int bit_idx = id % bm_bit_per_blk;
	bm_clear_bit(bm_blk->buf, bit_idx);

	bcache_mark_dirty(bm_blk);
	bcache_release(bm_blk);
	return 0;
}

static inline bool _is_valid_locked(struct inode *i)
{
	return i->valid;
}

static inline void _set_valid_locked(struct inode *i)
{
	i->valid = true;
}

static inline void _clear_valid_locked(struct inode *i)
{
	i->valid = false;
}

static int _match_id_cb_locked(clist_node_t *node, void *args)
{
	uint32_t id = *(uint32_t *)args;
	struct inode *i = container_of(node, struct inode, node);
	int predicate = i->id == id;
	return predicate;
}

static struct inode *_cache_request_locked(uint32_t id)
{

	// MRU policy
	clist_node_t *i_node = clist_find(&icache, _match_id_cb_locked, &id);

	if (!i_node) {
		return NULL;
	}

	return container_of(i_node, struct inode, node);
}

static int _find_by_refcnt_cb_locked(clist_node_t *node,
				     [[maybe_unused]] void *args)
{
	struct inode *i = container_of(node, struct inode, node);
	int predicate = i->refcnt == 0;
	return predicate;
}

/**
 * @brief Performs a LRU scan to find reusable inode slot.
 */
static struct inode *_find_reusable_locked(void)
{
	// LRU policy
	clist_node_t *i_node = clist_find_rev(&icache,
					      _find_by_refcnt_cb_locked, NULL);

	if (!i_node) {
		return NULL;
	}

	return container_of(i_node, struct inode, node);
}

struct inode *inode_get(uint32_t id)
{
	struct inode *i;
	spinlock_lock(&cache_lock);

	i = _cache_request_locked(id);
	// HIT
	if (i) {
		i->refcnt++;
		// MRU
		clist_remove(&i->node);
		clist_push_front(&icache, &i->node);

		spinlock_unlock(&cache_lock);
		return i;
	}

	// MISS
	i = _find_reusable_locked();
	if (!i) {
		panic("BUFFER OVERFLOW: buffer all used");
	}

	i->id = id;
	i->refcnt = 1;
	_clear_valid_locked(i);

	spinlock_unlock(&cache_lock);
	return i;
}

int inode_lock(struct inode *i)
{
	mutex_lock(&i->m);
	if (!_is_valid_locked(i)) {

		// Fetch content from disk
		struct cblk *tab_blk = bcache_read(_get_blk(i->id));
		if (!tab_blk) {
			mutex_unlock(&i->m);
			return -1;
		}
		struct dinode *tab_entry = (struct dinode *)tab_blk->buf;
		int idx = _get_tab_idx(i->id);
		i->content = tab_entry[idx];

		bcache_release(tab_blk);
		_set_valid_locked(i);
	}
	return 0;
}

void inode_unlock(struct inode *i)
{
	mutex_unlock(&i->m);
}

static inline int _mark_dirty_locked(struct inode *i)
{
	struct cblk *tab_blk = bcache_read(_get_blk(i->id));
	if (!tab_blk) {
		return -1;
	}

	struct dinode *tab_entry = (struct dinode *)tab_blk->buf;
	int idx = _get_tab_idx(i->id);
	tab_entry[idx] = i->content;
	bcache_mark_dirty(tab_blk);
	bcache_release(tab_blk);
	return 0;
}

int inode_mark_dirty(struct inode *i)
{
	if (!mutex_is_lock(&i->m)) {
		panic("Trying to mark dirty without lock aquired");
	}
	return _mark_dirty_locked(i);
}

static int _release_dir_blk_locked(struct inode *i)
{

	int res;
	for (int idx = 0; idx < DINODE_DIR_ADDR; idx++) {
		uint32_t blkno = i->content.dirblkno[idx];
		if (blkno == 0) {
			continue;
		}

		i->content.dirblkno[idx] = 0;
		res = bfree(blkno);
		if (res < 0) {
			return res;
		}
	}

	return 0;
}

static int _release_indir_locked(struct inode *i)
{

	// One indirection lvl
	struct cblk *indir_blk = bcache_read(i->content.indir);
	if (!indir_blk) {
		return -1;
	}

	uint32_t *content = (uint32_t *)indir_blk->buf;
	for (uint32_t i = 0; i < DINODE_INDIR_ADDR; i++) {
		uint32_t blkno = content[i];
		if (blkno == 0) {
			continue;
		}

		content[i] = 0;
		bfree(blkno);
	}

	bcache_mark_dirty(indir_blk);
	bcache_release(indir_blk);
	return 0;
}

static inline int32_t _get_dir_blk(struct inode *i, uint32_t nth)
{
	uint32_t blkno = i->content.dirblkno[nth];
	if (blkno == 0) {
		blkno = balloc();
		if (blkno < 0) {
			return blkno;
		}
		i->content.dirblkno[nth] = blkno;
	}

	return blkno;
}

static inline int32_t _get_indir_blk(struct inode *i, uint32_t nth)
{
	// One indirection lvl
	struct cblk *indir_blk = bcache_read(i->content.indir);
	if (!indir_blk) {
		return -1;
	}

	uint32_t *content = (uint32_t *)indir_blk->buf;
	uint32_t blkno = content[nth];
	if (blkno == 0) {
		blkno = balloc();
		if (blkno < 0) {
			goto err_release;
		}
		content[nth] = blkno;
		bcache_mark_dirty(indir_blk);
	}
err_release:
	bcache_release(indir_blk);
	return blkno;
}

int32_t _get_data_locked(struct inode *i, uint32_t nth)
{
	int32_t res;
	if (nth >= DINODE_DIR_ADDR + DINODE_INDIR_ADDR) {
		return -1;
	}

	if (nth < DINODE_DIR_ADDR) {
		res = _get_dir_blk(i, nth);

	} else {
		res = _get_indir_blk(i, nth);
	}
	return res;
}

static int inode_truncate_locked(struct inode *i)
{
	int res;

	res = _release_dir_blk_locked(i);
	if (res < 0) {
		return res;
	}

	res = _release_indir_locked(i);
	if (res < 0) {
		return res;
	}

	i->content.size = 0;
	return 0;
}

int inode_truncate(struct inode *i)
{
	int res = inode_lock(i);
	if (res < 0) {
		return res;
	}

	res = inode_truncate_locked(i);
	if (res < 0) {
		goto out;
	}
	res = _mark_dirty_locked(i);

out:
	inode_unlock(i);
	return res;
}

struct inode *inode_create(enum inode_type type)
{
	int32_t id = _ibm_alloc();
	if (id < 0) {
		return NULL;
	}
	struct inode *i = inode_get(id);
	if (inode_lock(i) < 0) {
		goto err_put;
	}

	i->content.type = type;
	i->content.nblink = 1;

	if (_mark_dirty_locked(i) < 0) {
		// Failed to update node entry
		_clear_valid_locked(i);
		goto err_unlock;
	}

	inode_unlock(i);
	return i;

err_unlock:
	inode_unlock(i);
err_put:
	inode_put(i);
	_ibm_free(id);
	return NULL;
}

void inode_put(struct inode *i)
{
	spinlock_lock(&cache_lock);
	if (i->refcnt == 1) {
		spinlock_unlock(&cache_lock);

		mutex_lock(&i->m);
		if (_is_valid_locked(i) && i->content.nblink == 0) {
			if (inode_truncate_locked(i) < 0) {
				panic("truncate failed");
			}
			i->content.type = INODE_FREE;
			if (_mark_dirty_locked(i) < 0) {
				panic("dirty failed");
			}
			_ibm_free(i->id);
			_clear_valid_locked(i);
		}

		mutex_unlock(&i->m);
		spinlock_lock(&cache_lock);
	}
	i->refcnt--;
	spinlock_unlock(&cache_lock);
}

#define MIN(a, b) ((a) < (b) ? (a) : (b))

ssize_t inode_write(struct inode *i, bool user_src, const char *src,
		    uint32_t offset, size_t count)
{
	int res = 0;

	//NOTE: We allow file size extension when writing but not more than
	// the MAX_FILE_SIZE.
	if (offset > i->content.size || (offset + count) > MAX_FILE_SIZE) {
		return -1;
	}

	size_t done, m;

	for (done = 0; done < count; done += m, offset += m, src += m) {
		uint32_t blk_off = offset % FS_BLK_SIZE;
		// Byte to process in the block
		m = MIN(count - done, FS_BLK_SIZE - blk_off);

		int32_t dblkno = _get_data_locked(i, offset / FS_BLK_SIZE);
		if (dblkno < 0) {
			res = dblkno;
			goto out;
		}

		struct cblk *dblk = bcache_getblk(dblkno);
		if (!dblk) {
			res = -1;
			goto out;
		}

		if (user_src) {
			pte_t *root = process_active()->root_ptable;
			res = vpage_copyin(root, dblk->buf + blk_off, src, m);
			if (res < 0) {
				bcache_release(dblk);
				goto out;
			}

		} else {
			memcpy(dblk->buf + blk_off, src, m);
		}

		bcache_mark_dirty(dblk);
		bcache_release(dblk);
	}

out:
	// File growing is possible
	if (i->content.size < offset) {
		i->content.size = offset;
		inode_mark_dirty(i);
	}

	return done ? done : res;
}

ssize_t inode_read(struct inode *i, bool user_src, char *dst, uint32_t offset,
		   size_t count)
{
	//NOTE: read cannot read out of range of the current content
	if (offset + count > i->content.size) {
		return -1;
	}

	int res = 0;
	size_t done, m;

	for (done = 0; done < count; done += m, offset += m, dst += m) {
		uint32_t blk_off = offset % FS_BLK_SIZE;
		m = MIN(count - done, FS_BLK_SIZE - blk_off);

		int32_t dblkno = _get_data_locked(i, offset / FS_BLK_SIZE);
		if (dblkno < 0) {
			res = dblkno;
			goto out;
		}

		struct cblk *dblk = bcache_getblk(dblkno);
		if (!dblk) {
			res = -1;
			goto out;
		}
		if (user_src) {
			pte_t *root = process_active()->root_ptable;
			res = vpage_copyout(root, dst, dblk->buf + blk_off, m);
			if (res < 0) {
				bcache_release(dblk);
				goto out;
			}

		} else {
			memcpy(dblk->buf + blk_off, dst, m);
		}
		bcache_release(dblk);
	}

out:
	return done ? done : res;
}
