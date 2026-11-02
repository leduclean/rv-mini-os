#pragma once
#include <stddef.h>
#include <stdint.h>

#include <err.h>
#include <kinterface.h>

/**
 * @brief Sleep syscall used to make a process sleep.
 *
 * @param sec Number of sleeping seconds before the process wakes up.
 */
void sleep(uint32_t sec);

/**
 * @brief Exit syscall called to trigger the thread terminaison
 *
 * This function cleans the entire struct associated with the thread.
 *
 * @param code Terminaison code: 0 if normal, else error codes.
 */
void exit(uint8_t code);

/**
 * @brief Exit syscall called on a process terminaison.
 *
 * This function cleans the entire struct associated with the process
 * and all the thread associated with the process.
 *
 * @param code Terminaison code: 0 if normal, else error codes.
 */
void exit_group(uint8_t code);

/**
 * @brief Wait for the terminaison of any child, then reap it.
 *
 * @note Blocks the caller until one of its children becomes a zombie.
 *
 * @return Pid of the reaped child.
 */
int8_t wait(void);

/**
 * @brief Wait for the terminaison of a specific child, then reap it.
 *
 * @note Blocks the caller until this child becomes a zombie.
 *
 * @param pid Pid of the child to wait for.
 * @return Pid of the reaped child.
 */
int8_t waitpid(int8_t pid);

/**
 * @brief Dupplicate a process state making a copy of it execution
 *
 * @return The child pid in the parent, 0 in the child.
 * @retval -EAGAIN Process table full.
 * @retval -ENOMEM Out of memory.
 */
int8_t fork(void);

/**
 * @brief Create a new "child" in a manner similar to @p fork.
 *
 * @param entry The entry code of the process, can be NULL.
 * @param args  The args of the @entry point.
 * @param stack A pointer to the allocated stack.
 * @param flags Flags to specify which elements to copy.
 * @return The child pid.
 * @retval -EINVAL CLONE_VM without @p entry or @p stack.
 * @retval -EAGAIN Process table full.
 * @retval -ENOMEM Out of memory.
 */
int8_t clone(void *entry, void *args, void *stack, unsigned long flags);

/**
 * @brief Execute a process referred by @path, this cause 
 * the currently run to be replaced with a new program.
 *
 * @return Does not return on success.
 * @retval -ENOEXEC Bad elf.
 * @retval -E2BIG More than EXEC_MAX_ARGS arguments.
 * @retval -EFAULT Bad address.
 * @retval -ENAMETOOLONG An argument exceeds MAXNAME.
 * @retval -ENOMEM Out of memory.
 */
int exec(const char *path, char *const argv[]);

/**
 * @brief Open a file and returns the corresponding file descriptor number.
 *
 * @param path The path of the file.
 * @param flags The wanted flags for the user.
 * @return The fd number.
 * @retval -EINVAL @p path is not absolute.
 * @retval -ENOENT No such file and O_CREAT unset.
 * @retval -ENOTDIR A path component is not a directory.
 * @retval -ENAMETOOLONG Path or component too long.
 * @retval -EFAULT Bad address.
 * @retval -ENFILE System file table full.
 * @retval -EMFILE Process fd table full.
 * @retval -ENOSPC Disk full.
 * @retval -EIO Disk failure.
 */
int open(const char *path, unsigned long flags);

/**
 * @brief Close a file descriptor.
 *
 * @param fd The file descriptor to close.
 * @retval 0 Success.
 * @retval -EBADF @p fd is not open.
 */
int close(int fd);

/**
 * @brief Writes up to count @count bytes from the buffer at @buf 
 * to the file referred by @fd.
 *
 * @param fd The file descriptor idx.
 * @param buf The buffer to write from.
 * @param count The number of bytes to write.
 * @return Number of bytes written.
 * @retval -EBADF @p fd is not open for writing.
 * @retval -EINVAL Bad offset or @p count too big.
 * @retval -EFBIG File would exceed its max size.
 * @retval -EFAULT Bad address.
 * @retval -ENOSPC Disk full.
 * @retval -EIO Disk failure.
 */
ssize_t write(int fd, const char *buf, size_t count);

/**
 * @brief Read up to count @count bytes from the buffer at @buf 
 * to the file referred by @fd.
 *
 * @param fd The file descriptor idx.
 * @param buf The buffer to read into.
 * @param count The max number of bytes to read.
 * @return Number of bytes read.
 * @retval -EBADF @p fd is not open for reading.
 * @retval -EINVAL Range past the end of file or @p count too big.
 * @retval -EFAULT Bad address.
 * @retval -EIO Disk failure.
 */
ssize_t read(int fd, char *buf, size_t count);

/**
 * @brief Move the read/write file offset.
 *
 * @param fd File descriptor idx.
 * @param offset The offset to move to.
 * @param whence From where to move, see SEEK_X for whence values.
 * @return The new offset.
 * @retval -EBADF @p fd is not open.
 * @retval -EINVAL Bad @p whence or negative resulting offset.

 */
int64_t seek(int fd, int64_t offset, int whence);
