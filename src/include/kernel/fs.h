#pragma once

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <lib/types.h>

#include <asm/asm_defs.h>

#define FS_BLK_SIZE PAGE_SIZE /**< Hardcoded convention */

#define FS_MAX_PATH \
	(10 *       \
	 (MAXNAME + \
	  2)) /** Max size for a path, correspond to 10 Name + separator */

/** 
 * @name File system meta stub until mkfs
 *
 * @{*/
#define FS_NB_BLK 2048
#define FS_NB_RESERVED 1
#define FS_NB_SUPERBLK 1
#define FS_NB_IBM 2
#define FS_NB_DBM 2
#define FS_NB_INODE 5
#define FS_NB_DBLK                                                          \
	(FS_NB_BLK - FS_NB_INODE - FS_NB_IBM - FS_NB_DBM - FS_NB_SUPERBLK - \
	 FS_NB_RESERVED)
/** @}*/

/** 
 * @name File system blk location of the main components.
 *
 * @{*/
#define FS_SUPERBLK_START 1
#define FS_INODE_BM_START (FS_SUPERBLK_START + FS_NB_SUPERBLK)
#define FS_BLK_BM_START (FS_INODE_BM_START + FS_NB_IBM)
#define FS_INODE_TABLE_START (FS_BLK_BM_START + FS_NB_DBM)
#define FS_DATA_START (FS_INODE_TABLE_START + FS_NB_INODE)
/** @}*/

/**
 * @class fs_meta
 * @brief Future parsed structure of the mkfs.
 *
 */
struct fs_meta {
	uint32_t nb_blk;
	uint32_t blk_size;

	int nb_ibm;
	int nb_inode;
	int nb_dblk;
	int nb_dbm;
};

/**
 * @brief Init the file system.
 *
 * @return 0 on success, error code < 0 on FAILURE.
 */
int fs_init(void);

/**
 * @brief Gets the number of bit map bits per blocks.
 */
uint32_t fs_get_bm_bits_per_blk(void);

/**
 * @brief Gets the number of data bit map blocks.
 */
uint32_t fs_get_nb_dbm(void);

/**
 * @brief Gets the number of inode bit map blocks.
 */
uint32_t fs_get_nb_ibm(void);

bool fs_blk_is_data_blk(int blkno);

/**
 * @brief Create a device node.
 *
 * @param path Path to the node.
 * @param dev_id The device id of the node.
 * @return 0 on SUCCESS, error code < 0 on FAILURE.
 */
int fs_mknod(const char *path, int dev_id);

/* Forwards declatations to break circular import */
struct inode;
enum inode_type : uint8_t;

/**
 * @brief Retriver an inode from a path in the file system tree.
 *
 * @param path Path of the inode.
 * @param flags See the O_X flags of the kinterface header.
 * @param leaf_type Inode of the leaf type created if the flags contains O_CREAT
 */
struct inode *fs_inode_from_path(const char *path, unsigned long flags,
				 enum inode_type leaf_type);
