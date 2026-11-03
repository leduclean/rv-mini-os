#include <stdint.h>

#include <lib/bitmap.h>
#include <lib/err.h>
#include <lib/string.h>

#include <kernel/balloc.h>
#include <kernel/bcache.h>
#include <kernel/fs.h>
#include <kernel/fs_layout.h>

static inline uint32_t bit_to_blkno(uint32_t bit_idx, uint32_t bbm_nb)
{
	return bit_idx + (bbm_nb * FS_BM_BITS_PER_BLK) + sb.data_start;
}

static int32_t bbm_alloc(void)
{
	for (uint32_t i = 0; i < sb.nb_dbm; i++) {
		uint32_t bm_blk_no = sb.dbm_start + i;

		struct cblk *bm_blk = bcache_read(bm_blk_no);
		if (!bm_blk) {
			return -EIO;
		}

		int bit_idx = bm_find_first_zero(bm_blk->buf,
						 FS_BM_BITS_PER_BLK);
		if (bit_idx >= 0) {
			uint32_t blkno = bit_to_blkno(bit_idx, i);

			if (blkno >= sb.nb_blk) {
				/* Next are zero bit are not associated with a block */
				bcache_release(bm_blk);
				break;
			}

			bm_set_bit(bm_blk->buf, bit_idx);
			bcache_mark_dirty(bm_blk);
			bcache_release(bm_blk);
			return blkno;
		}

		bcache_release(bm_blk);
	}

	return -ENOSPC;
}

static int bbm_free(uint32_t blkno)
{
	// Only data block free are allowed
	if (blkno < sb.data_start || blkno >= sb.data_start + sb.nb_dblk) {
		return -EINVAL;
	}

	int idx = blkno - sb.data_start;
	uint32_t bm_blk_no = sb.dbm_start + idx / FS_BM_BITS_PER_BLK;

	struct cblk *bm_blk = bcache_read(bm_blk_no);
	if (!bm_blk) {
		return -EIO;
	}

	int bit_idx = idx % FS_BM_BITS_PER_BLK;
	bm_clear_bit(bm_blk->buf, bit_idx);

	bcache_mark_dirty(bm_blk);
	bcache_release(bm_blk);
	return 0;
}

int32_t balloc(void)
{
	int32_t blkno = bbm_alloc();
	if (blkno < 0)
		return blkno;

	struct cblk *b = bcache_read(blkno);
	if (!b) {
		bbm_free(blkno);
		return -EIO;
	}

	memset(b->buf, 0, FS_BLK_SIZE);
	bcache_mark_dirty(b);
	bcache_release(b);
	return blkno;
}

int bfree(uint32_t blkno)
{
	bcache_invalidate(blkno);
	return bbm_free(blkno);
};
