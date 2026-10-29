#pragma once
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <lib/types.h>

#include <asm/asm_defs.h>

#define FS_BLK_SIZE PAGE_SIZE /**< Hardcoded convention */

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

#define FS_CONSOLE 0 /* Define the dev_id of the console */

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

struct file;

typedef ssize_t (*write_cb)(struct file *f, const char *buf, size_t count);
typedef ssize_t (*read_cb)(struct file *f, char *buf, size_t count);

/**
 * @class file
 * @brief File structure, this can be detained by multiple processes 
 * if the fork() syscall is used.
 *
 */
struct file {
	uint32_t refcnt; /**< Fork process keep might keep the same fd. */
	uint32_t inum; /**< Inode number of the file */
	uint32_t off;
	uint8_t mode;

	write_cb w_op;
	read_cb r_op;
};

extern struct fs_meta superblock;

/**
 * @brief Init the file system.
 *
 * @return 0 on success, error code < 0 on FAILURE.
 */
int fs_init(void);

/**
 * @brief Alloc a new file descriptor in the open file table.
 */
struct file *file_alloc(void);

static inline uint32_t fs_get_block_size(void)
{
	return superblock.blk_size;
}

static inline uint32_t fs_get_bm_bits_per_blk(void)
{
	return superblock.blk_size * CHAR_BIT;
}

static inline uint32_t fs_get_nb_dbm(void)
{
	return superblock.nb_dbm;
}

static inline uint32_t fs_get_nb_ibm(void)
{
	return superblock.nb_ibm;
}

static inline bool fs_blk_is_data_blk(int blkno)
{
	return FS_DATA_START <= blkno &&
	       blkno < FS_DATA_START + superblock.nb_dblk;
}

void fs_map_dev_cb(int dev_id, write_cb w_op, read_cb r_op);

/**
 * @brief Get a new reference to this file, typically when forking a task.
 *
 * @param f A pointer to the file to obtain a reference.
 */
void fs_get(struct file *f);

/**
 * @brief Put a reference to a file, typically called on a close() routine.
 *
 * @param f A pointer to the reference of the file to release.
 */
void fs_put(struct file *f);

/**
 * @brief Kernel primitive to open a file and get the associate file structure.
 *
 * @param path The path of 
 * @param flags @p Open System flags.
 */
struct file *fs_open(char *path, unsigned long flags);
