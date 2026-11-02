/**
 * @file
 * @brief This file declare basic user progs that test the behaviour of each syscall.
 */

#include <stddef.h>
#include <stdint.h>

#include <kinterface.h>
#include <string.h>

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

void cmd_segfault_test([[maybe_unused]] int argc,
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

#define FS_TEST_FILE "/path/to/test"

void cmd_fs_io_test([[maybe_unused]] int argc,
		    [[maybe_unused]] char *const argv[])
{
	int8_t res;
	const char writen[] = "test\n";
	char readed[sizeof(writen)];

	int fd = open(FS_TEST_FILE, O_CREAT | O_W | O_R);
	if (fd < 0) {
		TEST_ERROR("open failed: %d", fd);
	}

	res = write(fd, writen, sizeof(writen));
	if (res < 0) {
		TEST_ERROR("write failed: %d", res);
	}

	int64_t off = seek(fd, 0, SEEK_SET);
	if (off != 0) {
		TEST_ERROR("seek to 0 returned %ld", off);
	}

	res = read(fd, readed, sizeof(readed));
	if (res < 0) {
		TEST_ERROR("read failed: %d", res);
	}

	if (strcmp(writen, readed) != 0) {
		TEST_ERROR("read back \"%s\", expected \"%s\"", readed, writen);
	}

	TEST_SUCCESS();

	close(fd);
}

void cmd_fs_close_test([[maybe_unused]] int argc,
		       [[maybe_unused]] char *const argv[])
{
	int8_t res;
	char sink[10];

	res = close(10);
	if (res == 0) {
		TEST_ERROR("close on unopened fd succeeded");
	}

	res = close(STDIN);
	if (res < 0) {
		TEST_ERROR("close stdin failed: %d", res);
	}

	res = read(STDIN, sink, sizeof(sink));
	if (res == 0) {
		TEST_ERROR("read on closed stdin returned 0");
	}

	TEST_SUCCESS();
}

void cmd_fs_seek_test([[maybe_unused]] int argc,
		      [[maybe_unused]] char *const argv[])
{
	int64_t off;

	int fd = open(FS_TEST_FILE, O_CREAT | O_W | O_R);
	if (fd < 0) {
		TEST_ERROR("open failed: %d", fd);
	}

	off = seek(fd, -10, SEEK_SET);
	if (off != -1) {
		TEST_ERROR("seek -10 SET returned %ld, expected -1", off);
	}

	off = seek(fd, 10, SEEK_SET);
	if (off != 10) {
		TEST_ERROR("seek 10 SET returned %ld, expected 10", off);
	}

	off = seek(fd, -10, SEEK_CUR);
	if (off != 0) {
		TEST_ERROR("seek -10 CUR returned %ld, expected 0", off);
	}

	off = seek(fd, -10, SEEK_CUR);
	if (off != -1) {
		TEST_ERROR("seek -10 CUR returned %ld, expected -1", off);
	}

	off = seek(fd, 10, SEEK_END);
	if (off < 0) {
		TEST_ERROR("seek 10 END failed: %ld", off);
	}

	TEST_SUCCESS();
}

void cmd_stream_test([[maybe_unused]] int argc,
		     [[maybe_unused]] char *const argv[])
{
	int8_t res;
	char ubuf[1];

	for (;;) {
		res = read(1, ubuf, 1);
		if (res < 0) {
			TEST_ERROR("read failed: %d", res);
		}

		res = write(1, ubuf, 1);
		if (res < 0) {
			TEST_ERROR("write failed: %d", res);
		}
	}
}
