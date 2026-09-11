#include "syscall.h"
#include "apps.h"
#include "minilib/stdio.h"

#define SLEEP_TIME 2

__attribute__((section(".user_text"))) void sleep_call()
{
	sleep(SLEEP_TIME);
}

__attribute__((section(".user_text"))) void fork_test()
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
