#include "syscall.h"
#include "syscall_id.h"
#include "minilib/types.h"

extern unsigned long usyscall(unsigned long n, unsigned long a, unsigned long b,
			      unsigned long c, unsigned long d, unsigned long e,
			      unsigned long f);

static inline long _syscall0(long n)
{
	return usyscall(n, 0, 0, 0, 0, 0, 0);
}
static inline long _syscall1(long n, long a)
{
	return usyscall(n, a, 0, 0, 0, 0, 0);
}

static inline long _syscall2(unsigned long n, unsigned long a, unsigned long b)
{
	return usyscall(n, a, b, 0, 0, 0, 0);
}

void sleep(uint32_t sec)
{
	_syscall1(SYS_SLEEP, sec);
}
void exit(int code)
{
	_syscall1(SYS_EXIT, code);
}

int8_t wait()
{
	return _syscall0(SYS_WAIT);
}

int8_t wait_pid(int8_t pid)
{
	return _syscall1(SYS_WAIT, pid);
}

uint8_t getpid()
{
	return _syscall0(SYS_GETPID);
}

int8_t fork()
{
	return _syscall0(SYS_FORK);
}
