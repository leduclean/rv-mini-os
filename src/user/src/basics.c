/**
 * @file
 * @brief Basic interactive user programs.
 */

#include <stdint.h>

#include "cmd_registry.h"
#include "syscall.h"
#include "test.h"

void cmd_stream([[maybe_unused]] int argc, [[maybe_unused]] char *const argv[])
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
