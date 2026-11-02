/**
 * @file
 * @brief Process tests: sleep, fork, exec, segfault.
 */

#include <stddef.h>
#include <stdint.h>

#include <kinterface.h>

#include "cmd_registry.h"
#include "syscall.h"
#include "test.h"

#define SLEEP_TIME 2

void cmd_sleep_call([[maybe_unused]] int argc,
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

	// First write after the fork: this store is what faults, on both sides.
	marker = (res == 0) ? 1 : 2;

	// Read back through the stack. Each side must see its own value: if the
	// page were still shared, both would sleep the same amount of time.
	sleep(marker);

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
		char func[15] = "sleep_call";
		// Exec a child process
		pid = exec(func, NULL);
		if (pid < 0) {
			TEST_ERROR("exec failed: %d", pid);
		}
	} else {
		waitpid(pid);
		char ubuf[27] = "[TEST] fork exec success \n";

		int res;
		res = write(1, ubuf, 27);
		if (res < 0) {
			TEST_ERROR("write failed: %d", res);
		}
	}

	TEST_SUCCESS();
}

void cmd_exec_test([[maybe_unused]] int argc,
		   [[maybe_unused]] char *const argv[])
{
	int8_t res;
	res = exec("sleep_call", NULL);
	if (res < 0) {
		TEST_ERROR("exec failed: %d", res);
	}
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
