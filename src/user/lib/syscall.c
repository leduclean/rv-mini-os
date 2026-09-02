#include "syscall.h"
#include "syscall_id.h"

extern long syscall(long n, long a, long b, long c, long d, long e, long f);
static inline long _syscall1(long n, long a)
{
	return syscall(n, a, 0, 0, 0, 0, 0);
}
static inline long _syscall2(long n, long a, long b)
{
	return syscall(n, a, b, 0, 0, 0, 0);
}

void sleep(uint32_t sec)
{
	_syscall1(SYS_SLEEP, sec);
}
