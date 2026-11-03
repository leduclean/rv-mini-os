/**
 * @file
 * @brief Define the file kernel API of the fs.
 *
 * A file reference is acquired with file_open() or file_clone(), shared with
 * file_get() and dropped with file_put(). The last file_put() releases the
 * inode.
 */
#pragma once

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <arch/asm_defs.h>
#include <user/kinterface.h>

#define FILE_DEV_CONSOLE 0 /* Define the dev_id of the console */

struct file;
typedef ssize_t (*write_cb)(struct file *f, const char *buf, size_t count);
typedef ssize_t (*read_cb)(struct file *f, char *buf, size_t count);

/**
 * @class file
 * @brief File structure, this can be detained by multiple tasks 
 * if the fork() syscall is used.
 *
 */
struct file {
	uint32_t refcnt; /**< Fork task keep might keep the same fd. */
	struct inode *i; /**< Inode of the file */
	int64_t off; /** Should never be < 0 */
	uint8_t mode;

	write_cb w_op;
	read_cb r_op;
};

/**
 * @brief Kernel primitive to open a file and get the associate file structure.
 *
 * @param path The path of the file.
 * @param flags @p Open System flags.
 * @return The opened file.
 * @retval ERR_PTR(-ENFILE) File table full.
 * @retval ERR_PTR(-Exxx) See fs_inode_from_path().
 */
struct file *file_open(const char *path, unsigned long flags);

/**
 * @brief Alloc an independant copy of a file descriptor.
 *
 * @param f The cloned file that has the same inode and the same mode. 
 * @return The copy.
 * @retval NULL Table full.
 */
struct file *file_clone(const struct file *f);

/**
 * @brief Get a new reference to this file, typically when forking a task.
 *
 * @param f A pointer to the file to obtain a reference.
 * @return The same structure.
 */
struct file *file_get(struct file *f);

/**
 * @brief Put a reference to a file, typically called on a close() routine.
 *
 * The inode reference is also released if the file is not owned anymore.
 *
 * @param f A pointer to the reference of the file to release.
 */
void file_put(struct file *f);

/**
 * @brief Move the read write file offset.
 *
 * @param f The file to seek in.
 * @param Offset offset to move.
 * @param whence From where to moove.
 * @return The new offset.
 * @retval -EINVAL Bad @p whence or negative resulting offset.
 * @retval -EIO Disk failure.
 */
int64_t file_seek(struct file *f, int64_t offset, int whence);

/**
 * @brief Map a device to it's callback read and write function.
 *
 * @param dev_id The device id.
 * @param w_op Write callback.
 * @param r_op Read callback.
 */
void file_map_dev_cb(int dev_id, write_cb w_op, read_cb r_op);
