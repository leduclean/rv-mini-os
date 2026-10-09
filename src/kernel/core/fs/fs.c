#include <kernel/fs.h>

// Stub structure for now
struct fs_meta superblock = {
	.nb_blk = FS_NB_BLK,
	.blk_size = FS_BLK_SIZE,

	.nb_ibm = FS_NB_IBM,
	.nb_inode = FS_NB_INODE,

	.nb_dblk = FS_NB_DBLK,
	.nb_dbm = FS_NB_DBM,
};
