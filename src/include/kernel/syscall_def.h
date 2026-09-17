#pragma once

/* @brief Centralized syscalls definition.
 *
 * This is used to preprocess enum variant, function
 * definition.
 *
 * @{ */
#define SYSCALLS(X)                                                       \
	X(SLEEP, void, sleep, (uint32_t sec))                             \
	X(EXIT, void, exit, (uint8_t code))                               \
	X(WAIT, int8_t, wait, (void))                                     \
	X(WAITPID, int8_t, waitpid, (int8_t pid))                         \
	X(GETPID, uint8_t, getpid, (void))                                \
	X(FORK, int8_t, fork, (void))                                     \
	X(EXEC, int, exec, (const char *path, char *const argv[]))        \
	X(WRITE, ssize_t, write, (int fd, const char *buf, size_t count)) \
	X(READ, ssize_t, read, (int fd, char *buf, size_t count))
/* }@*/

#define X_ENUM(name, ...) SYS_##name,
enum { SYSCALLS(X_ENUM) SYS_NBR };
#undef X_ENUM
