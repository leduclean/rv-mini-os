#pragma once

#include <stddef.h>
#include <stdint.h>

/* @brief Centralized syscalls definition.
 *
 * This is used to preprocess enum variant, function
 * definition.
 *
 * @{ */
#define SYSCALLS(X)                                                    \
	X(void, sleep, (uint32_t sec))                                 \
	X([[noreturn]] void, exit, (uint8_t code))                     \
	X([[noreturn]] void, exit_group, (uint8_t code))               \
	X(int8_t, wait, (void))                                        \
	X(int8_t, waitpid, (int8_t pid))                               \
	X(uint8_t, getpid, (void))                                     \
	X(int8_t, clone,                                               \
	  (void *entry, void *args, void *stack, unsigned long flags)) \
	X(int8_t, fork, (void))                                        \
	X(int, exec, (const char *path, char *const argv[]))           \
	X(int, open, (const char *path, unsigned long flags))          \
	X(ssize_t, write, (int fd, const char *buf, size_t count))     \
	X(ssize_t, read, (int fd, char *buf, size_t count))            \
	X(int, close, (int fd))                                        \
	X(int64_t, seek, (int fd, int64_t offset, int whence))         \
	/* }@*/

#define X_ENUM(_ret_type, name, ...) SYS_##name,
enum { SYSCALLS(X_ENUM) SYS_NBR };
#undef X_ENUM
