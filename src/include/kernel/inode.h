/**
 * @file
 * @brief Inode interface for the file system. Disk commiting is bcache job.
 */

#pragma once

#include <stdatomic.h>
#include <stddef.h>
#include <stdint.h>

#include <lib/container.h>

#include <kernel/mutex.h>
#include <user/kinterface.h>

enum inode_type : uint8_t {
	INODE_FREE,
	INODE_DIRECTORY,
	INODE_FILE,
	INODE_SPECIAL_FILE,
	INODE_DEVICE,
};

#define DINODE_DIR_ADDR 12
#define DINODE_INDIR_ADDR (FS_BLK_SIZE / sizeof(uint32_t))
#define MAX_FILE_SIZE (DINODE_DIR_ADDR + DINODE_INDIR_ADDR) * FS_BLK_SIZE
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
	uint32_t dirblkno[DINODE_DIR_ADDR];
	uint32_t indir;
};

static_assert(FS_BLK_SIZE % sizeof(struct dinode) == 0,
	      "dinode must divide block");

/**
 * @class inode
 * @brief In memory node.
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
 * @brief Init the inode cache structure and bitmap.
 *
 * @retval 0 Success.
 * @retval -EIO Disk failure.
 */
int inode_init(void);

/**
 * @brief Scan the bitmap looking for a free inode entry.
 *
 * @param type Content type of the new inode.
 *
 * @return The new inode.
 * @retval ERR_PTR(-ENOSPC) No free inode.
 * @retval ERR_PTR(-EIO) Disk failure.
 */
struct inode *inode_create(enum inode_type type);

/**
 * @brief Get a reference to the inode number @p id
 *
 * @param id Node id wanted.
 *
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
 *
 * @retval 0 Success.
 * @retval -EIO Disk failure.
 * @post inode_unlock on @i should be called after using the content.
 */
int inode_lock(struct inode *i);

/**
 * @brief Unlock the inode content.
 *
 * @note If the content is not valid, it will fetch from the disk.
 *
 * @param i A pointer to the inode to lock.
 *
 * @retval 0 Success.
 * @retval -EIO Disk failure.
 * @pre A call to this function follows an inode_lock() call on @i 
 */
void inode_unlock(struct inode *i);

/**
 * @brief Writes the in-memory inode back to its inode table block.
 *
 * Must be called after any change to @c i->content, before inode_unlock().
 *
 * @param i The inode.
 *
 * @retval 0 Success.
 * @retval -EIO Disk failure.
 * @pre @c i->m held (inode_lock()).
 */
int inode_mark_dirty(struct inode *i);

/**
 * @brief Writes the inode data content with a provided buffer.
 *
 * @note Writing out of range will led to file size extension.
 *
 * @param i A pointer to the inode.
 * @param user_src Flag indicating weither the src is user provided.
 * @param src Pointer to the src buffer to write.
 * @param offset The offset in the data content.
 * @param n The number of bytes to write.
 *
 * @return Number of bytes written.
 * @retval -EINVAL @p offset is past the end of file.
 * @retval -EFBIG The file would grow past MAX_FILE_SIZE.
 * @retval -ENOSPC Disk full.
 * @retval -EIO Disk failure.
 * @retval -EFAULT Bad user address.
 *
 * @pre You must hold the content lock via inode_lock()
 */
ssize_t inode_write(struct inode *i, bool user_src, const char *src,
		    uint32_t offset, size_t n);

/**
 * @brief Read the inode data content with a provided buffer.
 *
 * @param i A pointer to the inode.
 * @param user_src Flag indicating weither the dst is user provided.
 * @param dst Pointer to the buffer to read in.
 * @param offset The offset in the data content.
 * @param n The number of bytes to read.
 *
 * @return Number of bytes read.
 * @retval -EINVAL Range past the end of file.
 * @retval -EIO Disk failure.
 * @retval -EFAULT Bad user address.
 *
 * @pre You must hold the content lock via inode_lock()
 */
ssize_t inode_read(struct inode *i, bool user_src, char *dst, uint32_t offset,
		   size_t count);
