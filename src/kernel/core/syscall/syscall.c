#include <lib/stdio.h>

#include <asm/cpu.h>

#include <kernel/process.h>
#include <kernel/scheduler.h>
#include <kernel/syscall.h>
#include <kernel/syscall_def.h>

typedef long (*syscall_fn)(unsigned long, unsigned long, unsigned long,
			   unsigned long, unsigned long, unsigned long);

#define X_TAB_ENTRY(enum, ret, name, args) \
	[SYS_##enum] = (syscall_fn)(void *)sys_##name,

static const syscall_fn syscall_tab[SYS_NBR] = { SYSCALLS(X_TAB_ENTRY) };

#undef X_TAB_ENTRY

long syscall_dispatch(unsigned long n, unsigned long a, unsigned long b,
		      unsigned long c, unsigned long d, unsigned long e,
		      unsigned long f)

{
	if (n >= SYS_NBR) {
		panic("Unsupported syscall");
	}
	return syscall_tab[n](a, b, c, d, e, f);
}
