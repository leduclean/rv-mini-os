#include "syscall.h"
#include "minilib/stddef.h"
#include "minilib/stdio.h"
#include "minilib/string.h"
#include "progs.h"

#define SLEEP_TIME 2

void sleep_call()
{
	for (;;) {
		sleep(SLEEP_TIME);
		printf("[U/INFO]: sleeped for %d s \n", SLEEP_TIME);
		printf("[U/INFO]: returned from kernel trap. \n");
	}
}
