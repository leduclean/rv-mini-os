/**
 * @file
 * @brief File system tests: open, read, write, seek, close.
 */

#include <stdint.h>

#include <kinterface.h>
#include <string.h>

#include "cmd_registry.h"
#include "err.h"
#include "syscall.h"
#include "test.h"

#define FS_TEST_FILE "/path/to/test"

void cmd_fs_io_test([[maybe_unused]] int argc,
		    [[maybe_unused]] char *const argv[])
{
	const char writen[] = "test\n";
	char readed[sizeof(writen)];

	int fd = open(FS_TEST_FILE, O_CREAT | O_W | O_R);
	if (fd < 0) {
		TEST_ERROR("open failed: %d", fd);
	}

	TEST_ASSERT_EQ(sizeof(writen), write(fd, writen, sizeof(writen)));

	TEST_ASSERT_EQ(0, seek(fd, 0, SEEK_SET));
	TEST_ASSERT_EQ(sizeof(readed), read(fd, readed, sizeof(readed)));
	TEST_ASSERT_EQ_STR(writen, readed);

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

	TEST_ASSERT_EQ(0, close(STDIN));

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

	TEST_ASSERT_EQ(-EINVAL, seek(fd, -10, SEEK_SET));
	TEST_ASSERT_EQ(10, seek(fd, 10, SEEK_SET));
	TEST_ASSERT_EQ(0, seek(fd, -10, SEEK_CUR));
	TEST_ASSERT_EQ(-EINVAL, seek(fd, -10, SEEK_CUR));

	off = seek(fd, 10, SEEK_END);
	if (off < 0) {
		TEST_ERROR("seek 10 END failed: %ld", off);
	}

	TEST_SUCCESS();
}
