#pragma once
#include <stddef.h>
#include <stdint.h>

#include <lib/types.h>

/**
 * @brief Sleep syscall used to make a process sleep.
 *
 * @param sec Number of sleeping seconds before the process wakes up.
 */
void sleep(uint32_t sec);

/**
 * @brief Exit syscall called on a user process terminaison.
 *
 * This function cleans the entire struct associated with the process itself.
 *
 * @param code Terminaison code: 0 if normal, else error codes.
 */
void exit(int code);

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
int8_t wait_pid(int8_t pid);

/**
 * @brief Dupplicate a process state making a copy of it execution
 *
 * @return The pid of the child process in the parent flow or 0 
 * in the child flow.
 */
int8_t fork(void);

/**
 * @brief Execute a process referred by @path, this cause 
 * the currently run to be replaced with a new program.
 *
 * @return Does not returns on success, -1 on error.
 */
int exec(const char *path, char *const argv[]);

/**
 * @brief Writes up to count @count bytes from the buffer at @buf 
 * to the file referred by @fd.
 *
 * @param fd The file descriptor idx.
 * @param buf The buffer to write from.
 * @param count The number of bytes to write.
 * @return The number of byte written on success, -1 on error.
 */
ssize_t write(int fd, const char *buf, size_t count);

/**
 * @brief Read up to count @count bytes from the buffer at @buf 
 * to the file referred by @fd.
 *
 * @param fd The file descriptor idx.
 * @param buf The buffer to read into.
 * @param count The max number of bytes to read.
 * @return The number of byte readed on success, -1 on error.
 */
ssize_t read(int fd, char *buf, size_t count);
