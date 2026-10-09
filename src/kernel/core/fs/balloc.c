#include <limits.h>
#include <stdint.h>

#include <lib/bitmap.h>

#include <kernel/balloc.h>
#include <kernel/bcache.h>
#include <kernel/fs.h>

int32_t balloc(void)
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

int32_t bfree(int blkno)
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
