#include <stdint.h>

#include <lib/bitmap.h>
#include <lib/string.h>

#include <kernel/balloc.h>
#include <kernel/bcache.h>
#include <kernel/fs.h>

static int32_t _bbm_alloc(void)
{
	uint32_t nb_bmblk = fs_get_nb_dbm();
	uint32_t bm_bit_per_blk = fs_get_bm_bits_per_blk();

	for (uint32_t i = 0; i < nb_bmblk; i++) {
		uint32_t bm_blk_no = FS_BLK_BM_START + i;

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

static int _bbm_free(uint32_t blkno)
{
	// Only data block free are allowed
	if (!fs_blk_is_data_blk(blkno)) {
		return -1;
	}

	uint32_t bm_bit_per_blk = fs_get_bm_bits_per_blk();
	uint32_t bm_blk_no = FS_BLK_BM_START + blkno / bm_bit_per_blk;

	struct cblk *bm_blk = bcache_read(bm_blk_no);
	if (!bm_blk) {
		return -1;
	}

	int bit_idx = blkno % bm_bit_per_blk;
	bm_clear_bit(bm_blk->buf, bit_idx);

	bcache_mark_dirty(bm_blk);
	bcache_release(bm_blk);
	return 0;
}

int32_t balloc(void)
{
	int32_t blkno = _bbm_alloc();
	if (blkno < 0)
		return -1;

	struct cblk *b = bcache_read(blkno);
	if (!b) {
		_bbm_free(blkno);
		return -1;
	}

	memset(b->buf, 0, FS_BLK_SIZE);
	bcache_mark_dirty(b);
	bcache_release(b);
	return blkno;
}

int bfree(uint32_t blkno)
{
	bcache_invalidate(blkno);
	return _bbm_free(blkno);
};
