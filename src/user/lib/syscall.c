#include <stddef.h>

#include <lib/types.h>

#include <user/syscall.h>
#include <user/syscall_id.h>

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

static inline long _syscall2(long n, unsigned long a, unsigned long b)
{
	return usyscall(n, a, b, 0, 0, 0, 0);
}

static inline long _syscall3(unsigned long n, unsigned long a, unsigned long b,
			     unsigned long c)
{
	return usyscall(n, a, b, c, 0, 0, 0);
}

void sleep(uint32_t sec)
{
	_syscall1(SYS_SLEEP, sec);
}
void exit(int code)
{
	_syscall1(SYS_EXIT, code);
}

int8_t wait(void)
{
	return _syscall0(SYS_WAIT);
}

int8_t wait_pid(int8_t pid)
{
	return _syscall1(SYS_WAIT, pid);
}

uint8_t getpid(void)
{
	return _syscall0(SYS_GETPID);
}

int8_t fork(void)
{
	return _syscall0(SYS_FORK);
}

int exec(const char *path, char *const argv[])
{
	return _syscall2(SYS_EXEC, (unsigned long)path, (unsigned long)argv);
}

ssize_t write(int fd, const char *buf, size_t count)
{
	return _syscall3(SYS_WRITE, fd, (unsigned long)buf, count);
}

ssize_t read(int fd, char *buf, size_t count)
{
	return _syscall3(SYS_READ, fd, (unsigned long)buf, count);
}
