//TODO: would be cool to generate stub via SYSCALLS(X_STUB)
#include <stddef.h>
#include <stdint.h>

#include <lib/types.h>

#include <kernel/syscall_def.h>
#include <user/syscall.h>

extern unsigned long usyscall(unsigned long n, unsigned long a, unsigned long b,
			      unsigned long c, unsigned long d, unsigned long e,
			      unsigned long f);

#define SYSCALL_(n, a, b, c, d, e, f, ...) usyscall(n, a, b, c, d, e, f)
#define SYSCALL(sys, ...) SYSCALL_(sys, ##__VA_ARGS__, 0, 0, 0, 0, 0, 0)

void sleep(uint32_t sec)
{
	SYSCALL(SYS_SLEEP, sec);
}
void exit(int code)
{
	SYSCALL(SYS_EXIT, code);
}

int8_t wait(void)
{
	return SYSCALL(SYS_WAIT);
}

int8_t waitpid(int8_t pid)
{
	return SYSCALL(SYS_WAITPID, pid);
}

uint8_t getpid(void)
{
	return SYSCALL(SYS_GETPID);
}

int8_t fork(void)
{
	return SYSCALL(SYS_FORK);
}

int exec(const char *path, char *const argv[])
{
	return SYSCALL(SYS_EXEC, (unsigned long)path, (unsigned long)argv);
}

ssize_t write(int fd, const char *buf, size_t count)
{
	return SYSCALL(SYS_WRITE, fd, (unsigned long)buf, count);
}

ssize_t read(int fd, char *buf, size_t count)
{
	return SYSCALL(SYS_READ, fd, (unsigned long)buf, count);
}
