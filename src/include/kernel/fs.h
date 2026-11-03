#pragma once

#include <limits.h>
#include <stddef.h>
#include <stdint.h>

#include <arch/asm_defs.h>
#include <kernel/fs_layout.h>
#include <kernel/task.h>
#include <user/kinterface.h>

#define FS_MAX_PATH \
	(10 *       \
	 (MAXNAME + \
	  2)) /** Max size for a path, correspond to 10 Name + separator */

/**
 * @brief Init the file system. And check the disk format.
 *
 * @retval 0 Success.
 * @retval -ENOSPC No free inode for the root.
 * @retval -EIO Disk failure.
 */
int fs_init(void);

/**
 * @brief Superblock read from disk at fs_init(). Read-only after init.
 */
extern struct fs_meta sb;

/**
 * @brief Create a device node.
 *
 * @param path Path to the node.
 * @param dev_id The device id of the node.
 * @retval 0 Success.
 * @retval -Exxx See fs_inode_from_path().
 */
int fs_mknod(const char *path, int dev_id);

/* Forwards declatations to break circular import */
struct inode;

/**
 * @brief Retriver an inode from a path in the file system tree.
 *
 * @param path Path of the inode.
 * @param flags See the O_X flags of the kinterface header.
 * @param leaf_type Inode of the leaf type created if the flags contains O_CREAT
 * @return The referenced inode.
 *
 * @retval ERR_PTR(-EINVAL) @p path is not absolute.
 * @retval ERR_PTR(-ENOENT) A component is missing and O_CREAT is unset.
 * @retval ERR_PTR(-ENOTDIR) A component is not a directory.
 * @retval ERR_PTR(-ENAMETOOLONG) A component exceeds MAXNAME.
 * @retval ERR_PTR(-ENOSPC) Disk full.
 * @retval ERR_PTR(-EIO) Disk failure.A
 * @post You MUST call @p inode_put() on the obtained inode.
 */

struct inode *fs_inode_from_path(const char *path, unsigned long flags,
				 enum inode_type leaf_type);
