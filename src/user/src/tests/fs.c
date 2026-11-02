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
	if (off != -EINVAL) {
		TEST_ERROR("seek -10 SET returned %ld, expected %d", off,
			   -EINVAL);
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
	if (off != -EINVAL) {
		TEST_ERROR("seek -10 CUR returned %ld, expected %d", off,
			   -EINVAL);
	}

	off = seek(fd, 10, SEEK_END);
	if (off < 0) {
		TEST_ERROR("seek 10 END failed: %ld", off);
	}

	TEST_SUCCESS();
}
