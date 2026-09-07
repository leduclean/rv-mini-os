#include "syscall.h"
#include "syscall_id.h"

extern long usyscall(long n, long a, long b, long c, long d, long e, long f);
__attribute__((section(".user_text"))) static inline long _syscall1(long n,
								    long a)
{
	return usyscall(n, a, 0, 0, 0, 0, 0);
}

__attribute__((section(".user_text"))) static inline long
_syscall2(long n, long a, long b)
{
	return usyscall(n, a, b, 0, 0, 0, 0);
}

__attribute__((section(".user_text"))) void sleep(uint32_t sec)
{
	_syscall1(SYS_SLEEP, sec);
}
