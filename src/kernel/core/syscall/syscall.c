#include "syscall.h"
#include "process.h"
#include "scheduler.h"
#include "syscall_id.h"
#include <stdio.h>

//TODO: Use a syscall table instead of a large switch case.
long syscall_dispatch(long n, long a, long b, long c)
{
	uint8_t pid = process_active()->pid;
	switch (n) {
	case SYS_SLEEP:
		printf("[Kernel/INFO]: Sleeping for %lu seconds (pid %d) \n", a,
		       pid);
		sys_sleep(a);
		return 0;
	case SYS_EXIT: {
		printf("[Kernel/INFO]: Called EXIT on process (pid %d) \n",
		       pid);
		scheduler_terminate(a);
		return 0;
	}
	case SYS_WAIT: {
		uint8_t res;
		if (a == 0) {
			res = sys_wait();
		} else {
			res = sys_wait_pid(a);
		}
		return res;
	}
	case SYS_GETPID:
		return sys_get_pid();
	case SYS_FORK:
		return sys_fork();
	case SYS_WRITE:
		return sys_write(a, (const char *)b, c);
	case SYS_READ:
		return sys_read(a, (char *)b, c);
	default:
		return -1;
	}
}
