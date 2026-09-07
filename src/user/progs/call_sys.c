#include "syscall.h"
#include "progs.h"
#include "minilib/stdio.h"

#define SLEEP_TIME 2

__attribute__((section(".user_text"))) void sleep_call()
{
	volatile int x = 42;
	for (;;) {
		sleep(SLEEP_TIME);
	}
}
