#include "syscall.h"
#include "scheduler.h"
#include "syscall_id.h"
#include "minilib/stdio.h"

long syscall_dispatch(long n, long a, long b, long c)
{
	switch (n) {
	case SYS_SLEEP:
		printf("[Kernel/INFO]: Sleeping for %lu seconds \n", a);
		scheduler_sleep(a);
		return 0;
		//TODO: Plug more syscall
	default:
		return -1;
	}
}
