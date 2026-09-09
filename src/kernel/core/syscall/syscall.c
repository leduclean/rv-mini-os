#include "syscall.h"
#include "mmap.h"
#include "process.h"
#include "scheduler.h"
#include "sync.h"
#include "syscall_id.h"
#include "minilib/stdio.h"
#include "trap.h"
#include "minilib/stdint.h"

//TODO: Use a syscall table instead of a large switch case.
long syscall_dispatch(long n, long a, long b, long c)
{
	switch (n) {
	case SYS_SLEEP:
		printf("[Kernel/INFO]: Sleeping for %lu seconds \n", a);
		scheduler_sleep(a);
		return 0;
		//TODO: Plug more syscall
	case SYS_EXIT: {
		uint8_t pid = get_active_pid();
		printf("[Kernel/INFO]: Called EXIT for process %d \n", pid);
		scheduler_terminate(a);
		return 0;
	}
	case SYS_WAIT: {
		uint8_t res;
		if (a == 0) {
			res = wait();
		} else {
			res = wait_pid(a);
		}
		return res;
	}
	case SYS_GETPID:
		return get_active_pid();
	default:
		return -1;
	}
}
