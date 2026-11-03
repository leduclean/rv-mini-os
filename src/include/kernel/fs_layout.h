/**
 * @file
 * @brief This file entire file system layout.
 */
#pragma once

#include <limits.h>
#include <stdint.h>

#define MAXNAME 16 /**< Max size for a file token */

/** 
 * @name File system blk location of the main components.
 *
 * @{*/
#define FS_SUPERBLK 1 /**< Superblock number. */
#define FS_MAGIC_NUMBER 69 /**< Magic number. */
#define FS_BLK_SIZE 4096 /**< Block size. */
#define FS_ROOT_INODE_ID 1 /**< Root inode id */
#define FS_BM_BITS_PER_BLK \
	(FS_BLK_SIZE * CHAR_BIT) /**< Number of bit per blk in the fs */
/** @}*/

/**
 * @class fs_meta
 * @brief Parsed structure of the mkfs.
 */
struct fs_meta {
	uint32_t magic;
	uint32_t nb_blk;
	uint32_t blk_size;

	uint32_t nb_ibm;
	uint32_t nb_dbm;
	uint32_t nb_itab; /**< Inode table size, in blocks. */
	uint32_t nb_dblk;

	/* First block of each region, computed by mkfs. */
	uint32_t ibm_start;
	uint32_t dbm_start;
	uint32_t itab_start;
	uint32_t data_start;
};

/**
 * @class dirent
 * @brief Directory entry descriptor. 
 */
struct dirent {
	char name[MAXNAME];
	uint32_t inum;
};

enum inode_type : uint8_t {
	INODE_FREE,
	INODE_DIRECTORY,
	INODE_FILE,
	INODE_SPECIAL_FILE,
	INODE_DEVICE,
};

/**
 * @name Inode data block descriptor.
 * @{
 */
#define FS_INODE_DIR_DBLKNO \
	12 /**< Number of direct dblock number in the inode.*/
#define FS_INODE_INDIR_DBLKNO \
	(FS_BLK_SIZE /        \
	 sizeof(uint32_t)) /**< Number of block number in one indirect block. */
#define FS_MAX_FILE_SIZE                                \
	(FS_INODE_DIR_DBLKNO + FS_INODE_INDIR_DBLKNO) * \
		FS_BLK_SIZE /** Max allowed file size */
/** @} */

/**
 * @class dinode
 * @brief In disk node.
 *
 */
struct dinode {
	uint32_t size;
	enum inode_type type;
	uint8_t dev_id; /* INODE_DEVICE */
	uint32_t nblink;
	uint32_t dirblkno[FS_INODE_DIR_DBLKNO];
	uint32_t indir;
};

static_assert(FS_BLK_SIZE % sizeof(struct dinode) == 0,
	      "dinode must divide block");
