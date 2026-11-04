/**
 * @file
 * @brief Process tests: sleep, fork, exec, segfault.
 */

#include <stddef.h>
#include <stdint.h>

#include <err.h>
#include <kinterface.h>

#include "cmd_registry.h"
#include "syscall.h"
#include "test.h"

#define SLEEP_TIME 2

void cmd_sleep_test([[maybe_unused]] int argc,
		    [[maybe_unused]] char *const argv[])
{
	sleep(SLEEP_TIME);
	TEST_SUCCESS();
}

void cmd_fork_test([[maybe_unused]] int argc,
		   [[maybe_unused]] char *const argv[])
{
	// volatile: the compiler must really store it on the user stack, which
	// is the page fork() just marked copy on write.
	volatile int8_t marker = 0;
	int8_t res;

	res = fork();
	if (res < 0) {
		TEST_ERROR("fork failed: %d", res);
	}

	if (res == 0) {
		// First write after the fork: this store must fault and give the
		// child its own copy of the page.
		marker = 1;
		TEST_ASSERT_EQ(1, marker);
		exit(0);
	}

	// The child has exited: if the page were still shared, its write
	// would be visible here.
	waitpid(res);
	TEST_ASSERT_EQ(0, marker);

	// Parent side of the copy on write: its own first write must stick.
	marker = 2;
	TEST_ASSERT_EQ(2, marker);

	TEST_SUCCESS();
}

/**
 * @name Test constant for exec tests.
 * @{
 */
#define TEST_ARGC 5 /**< Number of expected argc */
#define TEST_ARGV                                              \
	{ "exec_success", "test", "is", "a", "success", NULL } \
	/**< Given and expected argv. */
/** @} */

void cmd_exec_success(int argc, char *const argv[])
{
	char *const argv_test[EXEC_MAX_ARGS] = TEST_ARGV;
	TEST_ASSERT_EQ(TEST_ARGC, argc);
	for (int i = 0; i < argc; i++) {
		TEST_ASSERT_EQ_STR(argv_test[i], argv[i]);
	}

	TEST_SUCCESS();
}

void cmd_fork_exec_test([[maybe_unused]] int argc,
			[[maybe_unused]] char *const argv[])
{
	int8_t pid;
	pid = fork();
	if (pid < 0) {
		TEST_ERROR("fork failed: %d", pid);
	}
	if (pid == 0) {
		char *const argv_test[EXEC_MAX_ARGS] = TEST_ARGV;
		pid = exec("/user.elf", argv_test);
		if (pid < 0) {
			TEST_ERROR("exec failed: %d", pid);
		}
	} else {
		waitpid(pid);
	}
}

void cmd_exec_test([[maybe_unused]] int argc,
		   [[maybe_unused]] char *const argv[])
{
	int8_t res;
	char *const argv_test[EXEC_MAX_ARGS] = TEST_ARGV;
	res = exec("/user.elf", argv_test);
	if (res < 0) {
		TEST_ERROR("exec failed: %d", res);
	}

	TEST_ASSERT_EQ(exec("/nofile", NULL), -ENOENT);
	TEST_ASSERT_EQ(exec("/dev/console", NULL), -ENOEXEC);
}

void cmd_segfault([[maybe_unused]] int argc,
		  [[maybe_unused]] char *const argv[])
{
	int8_t res;

	res = fork();
	if (res < 0) {
		TEST_ERROR("fork failed: %d", res);
	}
	if (res == 0) {
		// This will trigger a segfault
		uint32_t *marker = NULL;
		*marker = 0;
	} else {
		waitpid(res);
		// Deferencing a NULL pointer should results in a page faults
		sleep(2);
	}
}
