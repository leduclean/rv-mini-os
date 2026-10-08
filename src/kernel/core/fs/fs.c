#include <kernel/fs.h>

// Stub structure for now
struct fs_meta superblock = {
	.nb_blk = FS_NB_BLK,
	.blk_size = FS_BLK_SIZE,

	.nb_dblk = FS_NB_DBLK,
	.nb_bitmap = FS_NB_BM,
	.nb_inode = FS_NB_INODE,
};
