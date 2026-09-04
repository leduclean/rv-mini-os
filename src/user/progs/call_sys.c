#include "syscall.h"
#include "progs.h"

#define SLEEP_TIME 2

void sleep_call()
{
	for (;;) {
		sleep(SLEEP_TIME);
	}
}
