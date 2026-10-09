/**
 * @file
 * @brief Inode interface for the file system. Disk commiting is bcache job.
 */

#pragma once

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include <lib/container.h>

#include <kernel/fs.h>
#include <kernel/mutex.h>

enum inode_type : uint32_t {
	INODE_FREE,
	INODE_DIRECTORY,
	INODE_FILE,
	INODE_SPECIAL_FILE,
};
#define DINODE_MAX_DIR 12
/**
 * @class dinode
 * @brief In disk node.
 *
 */
struct dinode {
	uint64_t size;
	enum inode_type type;
	int nblink;
	int dirblkno[DINODE_MAX_DIR];
};
static_assert(FS_BLK_SIZE % sizeof(struct dinode) == 0,
	      "dinode must divide block");

/**
 * @class inode
 * @brief In memory node.
 *
 */
struct inode {

	/** Protected by the @cache_lock */
	int refcnt;
	uint32_t id;
	clist_node_t node;

	/* Provides exclusive access to 
     * the content below fetched from disk
     * and to it status */
	mutex_t m;
	bool valid;
	struct dinode content;
};

/**
 * @brief Scan the bitmap looking for a free inode entry.
 *
 * @param type Return the new corresponding inode with it content type updated.
 */
struct inode *inode_create(enum inode_type type);

/**
 * @brief Get a reference to the inode number @p id
 *
 * @param id Node id wanted.
 * @return The obtained structure, content might be undefined, please use
 * inode_lock() before using content.
 * @post inode_put() MUST be called after each inode_get()
 */
struct inode *inode_get(uint32_t id);

/**
 * @brief Release a reference obtained by inode_get().
 *
 * @note If you are the last owner of the inode, it will be truncate and release.
 * @param i Pointer to the inode to release.
 */
void inode_put(struct inode *i);

/**
 * @brief Lock the inode content.
 *
 * @note If the content is not valid, it will fetch from the disk.
 *
 * @param i A pointer to the inode to lock.
 * @return 0 on SUCCESS, error code < 0 if disk fetch failed.
 * @post inode_unlock on @i should be called after using the content.
 */
int inode_lock(struct inode *i);

/**
 * @brief Unlock the inode content.
 *
 * @note If the content is not valid, it will fetch from the disk.
 *
 * @param i A pointer to the inode to lock.
 * @return 0 on SUCCESS, error code < 0 if disk fetch failed.
 * @post inode_unlock on @i should be called after using the content.
 */
void inode_unlock(struct inode *i);

/**
 * @brief Writes the in-memory inode back to its inode table block.
 *
 * Must be called after any change to @c i->content, before inode_unlock().
 *
 * @param i The inode.
 * @return 0 on SUCCESS, error code < 0 if the inode table block could not be read.
 * @pre @c i->m held (inode_lock()).
 */
int inode_mark_dirty(struct inode *i);
