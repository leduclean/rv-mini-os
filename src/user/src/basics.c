/**
 * @file
 * @brief This file declare basic user progs that test the behaviour of each syscall.
 */

#include <stddef.h>
#include <stdint.h>

#include <kinterface.h>

#include "apps.h"
#include "syscall.h"
#include "test.h"

#define SLEEP_TIME 2

void sleep_call(void)
{
	sleep(SLEEP_TIME);
	TEST_SUCCESS();
}

void fork_test(void)
{
	// volatile: the compiler must really store it on the user stack, which
	// is the page fork() just marked copy on write.
	volatile int8_t marker = 0;
	int8_t res;

	res = fork();
	if (res < 0) {
		TEST_ERROR();
	}

	// First write after the fork: this store is what faults, on both sides.
	marker = (res == 0) ? 1 : 2;

	// Read back through the stack. Each side must see its own value: if the
	// page were still shared, both would sleep the same amount of time.
	sleep(marker);

	TEST_SUCCESS();
}

void fork_exec_test(void)
{
	int8_t pid;
	pid = fork();
	if (pid < 0) {
		TEST_ERROR();
	}
	if (pid == 0) {
		char func[15] = "sleep_call";
		// Exec a child process
		pid = exec(func, NULL);
		if (pid < 0) {
			TEST_ERROR();
		}
	} else {
		waitpid(pid);
		char ubuf[27] = "[TEST] fork exec success \n";

		int res;
		res = write(1, ubuf, 27);
		if (res < 0) {
			TEST_ERROR();
		}
	}

	TEST_SUCCESS();
}

void exec_test(void)
{
	int8_t res;
	res = exec("sleep_call", NULL);
	if (res < 0) {
		TEST_ERROR();
	}
}

void segfault_test(void)
{
	int8_t res;

	res = fork();
	if (res < 0) {
		TEST_ERROR();
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

void fs_io_test(void)
{
	int8_t res;
	const char writen[] = "test\n";
	char readed[sizeof(writen)];

	int fd = open(FS_TEST_FILE, O_CREAT | O_W | O_R);
	if (fd < 0) {
		TEST_ERROR();
	}

	res = write(fd, writen, sizeof(writen));
	if (res < 0) {
		TEST_ERROR();
	}

	int64_t off = seek(fd, 0, SEEK_SET);
	if (off != 0) {
		TEST_ERROR();
	}

	res = read(fd, readed, sizeof(readed));
	if (res < 0) {
		TEST_ERROR();
	}

	for (size_t i = 0; i < sizeof(readed); i++) {
		if (readed[i] != writen[i]) {
			TEST_ERROR();
		}
	}

	TEST_SUCCESS();

	close(fd);
}

void fs_close_test(void)
{
	int8_t res;
	char sink[10];

	res = close(10);
	if (res == 0) {
		TEST_ERROR();
	}

	res = close(STDIN);
	if (res < 0) {
		TEST_ERROR();
	}

	res = read(STDIN, sink, sizeof(sink));
	if (res == 0) {
		TEST_ERROR();
	}

	TEST_SUCCESS();
}

void fs_seek_test(void)
{
	int64_t off;

	int fd = open(FS_TEST_FILE, O_CREAT | O_W | O_R);
	if (fd < 0) {
		TEST_ERROR();
	}

	off = seek(fd, -10, SEEK_SET);
	if (off != -1) {
		TEST_ERROR();
	}

	off = seek(fd, 10, SEEK_SET);
	if (off != 10) {
		TEST_ERROR();
	}

	off = seek(fd, -10, SEEK_CUR);
	if (off != 0) {
		TEST_ERROR();
	}

	off = seek(fd, -10, SEEK_CUR);
	if (off != -1) {
		TEST_ERROR();
	}

	off = seek(fd, 10, SEEK_END);
	if (off < 0) {
		TEST_ERROR();
	}

	TEST_SUCCESS();
}

void stream_test(void)
{
	int8_t res;
	char ubuf[1];

	for (;;) {
		res = read(1, ubuf, 1);
		if (res < 0) {
			TEST_ERROR();
		}

		res = write(1, ubuf, 1);
		if (res < 0) {
			TEST_ERROR();
		}
	}
}
