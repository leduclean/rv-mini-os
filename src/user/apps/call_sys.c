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
	int8_t res;

	res = fork();
	if (res < 0) {
		return exit(1);
	} else if (res == 0) {
		sleep(1);
	} else {
		sleep(2);
	}
}
