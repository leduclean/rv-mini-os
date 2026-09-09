#include "syscall.h"
#include "process.h"
#include "scheduler.h"
#include "syscall_id.h"
#include "minilib/stdio.h"

//TODO: Use a syscall table instead of a large switch case.
long syscall_dispatch(long n, long a, long b, long c)
{
	switch (n) {
	case SYS_SLEEP:
		printf("[Kernel/INFO]: Sleeping for %lu seconds \n", a);
		sys_sleep(a);
		return 0;
	case SYS_EXIT: {
		uint8_t pid = get_active_pid();
		printf("[Kernel/INFO]: Called EXIT for process %d \n", pid);
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
	default:
		return -1;
	}
}
