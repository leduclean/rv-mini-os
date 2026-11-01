/**
 * @file
 * @brief Define the file kernel API of the fs.
 */
#pragma once

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <user/kinterface.h>

#include <arch/asm_defs.h>

#define FILE_DEV_CONSOLE 0 /* Define the dev_id of the console */

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
	int64_t off; /** Should never be < 0 */
	uint8_t mode;

	write_cb w_op;
	read_cb r_op;
};

/**
 * @brief Alloc a new file descriptor in the open file table.
 */
struct file *file_alloc(void);

/**
 * @brief Map a device to it's callback read and write function.
 *
 * @param dev_id The device id.
 * @param w_op Write callback.
 * @param r_op Read callback.
 */
void file_map_dev_cb(int dev_id, write_cb w_op, read_cb r_op);

/**
 * @brief Alloc an independant copy of a file descriptor.
 *
 * @param f The cloned file that has the same inode and the same mode. 
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
 * @param f A pointer to the reference of the file to release.
 */
void file_put(struct file *f);

/**
 * @brief Kernel primitive to open a file and get the associate file structure.
 *
 * @param path The path of 
 * @param flags @p Open System flags.
 */
struct file *file_open(const char *path, unsigned long flags);

/**
 * @brief Move the read write file offset.
 *
 * @param f The file to seek in.
 * @param Offset offset to move.
 * @param whence From where to moove.
 * @return Resulting offset in bytes on SUCCESS, error code < 0 on FAILURE.
 */
int64_t file_seek(struct file *f, int64_t offset, int whence);
