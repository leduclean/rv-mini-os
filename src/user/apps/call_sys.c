#include "syscall.h"
#include "apps.h"
#include <stddef.h>

#define SLEEP_TIME 2

void sleep_call()
{
	sleep(SLEEP_TIME);
}

void fork_test()
{
	// volatile: the compiler must really store it on the user stack, which
	// is the page fork() just marked copy on write.
	volatile int8_t marker = 0;
	int8_t res;

	res = fork();
	if (res < 0) {
		return exit(1);
	}

	// First write after the fork: this store is what faults, on both sides.
	marker = (res == 0) ? 1 : 2;

	// Read back through the stack. Each side must see its own value: if the
	// page were still shared, both would sleep the same amount of time.
	sleep(marker);
}

void segfault_test()
{
	int8_t res;

	res = fork();
	if (res < 0) {
		return exit(1);
	}
	if (res == 0) {
		// This will trigger a segfault
		uint32_t *marker = NULL;
		*marker = 0;
	} else {
		wait_pid(res);
		// Deferencing a NULL pointer should results in a page faults
		sleep(2);
	}
}

void write_test()
{
	int8_t res;
	char ubuf[23] = "[TEST] write success \n";

	res = write(1, ubuf, 23);
	if (res < 0) {
		return exit(1);
	}
}

void read_test()
{
	int8_t res;
	char ubuf[10];

	res = read(1, ubuf, 10);
	if (res < 0) {
		return exit(1);
	}

	res = write(1, ubuf, 10);
	if (res < 0) {
		return exit(1);
	}
}

void stream_test()
{
	int8_t res;
	char ubuf[1];

	for (;;) {
		res = read(1, ubuf, 1);
		if (res < 0) {
			return exit(1);
		}

		res = write(1, ubuf, 1);
		if (res < 0) {
			return exit(1);
		}
	}
}
