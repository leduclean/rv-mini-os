/**
 * @file
 * @brief Registry of the commands the shell can run.
 */

#pragma once

#include <kinterface.h>

/**
 * @brief Command registry.
 *
 * To add a command: X(name, desc, usage) here, then define cmd_name().
 */
#define CMD_LIST(X)                                                        \
	X(help, "Show all the commands", "help")                           \
	X(sleep_call, "Sleep user system call test", "sleep_call")         \
	X(fork_test, "Test the page table copy with CoW", "fork_test")     \
	X(exec_test, "Exec the sleep_call app", "exec_test")               \
	X(fork_exec_test, "Fork then exec in the child", "fork_exec_test") \
	X(segfault_test, "Test the segfault handling", "segfault_test")    \
	X(fs_io_test, "Read write roundtrip", "fs_io_test")                \
	X(fs_close_test, "Test close behavior on a fd", "fs_close_test")   \
	X(fs_seek_test, "Test the seek syscall on a fd", "fs_seek_test")   \
	X(stream_test, "Stream console input back with read/write",        \
	  "stream_test")                                                   \
	X(thread_test, "Test the thread api", "thread_test")

typedef void (*cmd_func_t)(char *const argv[]);

#define X_DECL(n, d, u) void cmd_##n(char *const argv[]);
CMD_LIST(X_DECL)
#undef X_DECL

struct cmd_desc {
	cmd_func_t f;
	char *const argv[EXEC_MAX_ARGS];
	bool background;
};

/**
 * @brief Parse and run a command line.
 *
 * @param line NUL-terminated line, tokenized in place.
 * @param[out] desc The description to fill.
 * @retval 0 Success.
 * @retval -EINVAL Empty line.
 * @retval -ENOENT Unknown command.

 */
int cmd_get_desc(char *line, struct cmd_desc *desc);
