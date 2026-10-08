#pragma once
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <asm/asm_defs.h>

/**
 * @class fs_meta
 * @brief Future parsed structure of the mkfs.
 *
 */
struct fs_meta {
	uint32_t nb_blk;
	uint32_t blk_size;

	int nb_dblk;
	int nb_bmblk;
	int nb_inode;
};

#define FS_BLK_SIZE PAGE_SIZE /**< Hardcoded convention */

/** 
 * @name File system meta stub until mkfs
 *
 * @{*/
#define FS_NB_BLK 2048
#define FS_NB_RESERVED 1
#define FS_NB_SUPERBLK 1
#define FS_NB_INODE 3
#define FS_NB_BM 2
#define FS_NB_DBLK \
	(FS_NB_BLK - FS_NB_INODE - FS_NB_BM - FS_NB_SUPERBLK - FS_NB_RESERVED)
/** @}*/

/** 
 * @name File system blk location of the main components.
 *
 * @{*/
#define FS_SUPERBLK_START 1
#define FS_INODE_BM_START (FS_SUPERBLK_START + FS_NB_SUPERBLK)
#define FS_BLK_BM_START (FS_INODE_BM_START + FS_NB_INODE)
#define FS_INODE_TABLE_START (FS_BLK_BM_START + FS_NB_BM)
#define FS_DATA_START (FS_INODE_TABLE_START + FS_NB_INODE)
/** @}*/

extern struct fs_meta superblock;

static inline uint32_t fs_get_block_size(void)
{
	return superblock.blk_size;
}

static inline uint32_t fs_get_bm_bits_per_blk(void)
{
	return superblock.blk_size * CHAR_BIT;
}

static inline uint32_t fs_get_nb_bmblk(void)
{
	return superblock.nb_bmblk;
}

static inline bool fs_blk_is_data_blk(int blkno)
{
	return FS_DATA_START <= blkno &&
	       blkno < FS_DATA_START + superblock.nb_dblk;
}
