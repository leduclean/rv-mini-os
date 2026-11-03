#include "syscall.h"

#include <stddef.h>

#include <syscall_def.h>

/**
 * @brief Define all the user side syscall.
 *
 * @param ret Return type of the syscall.
 * @param name Name of the syscall.
 * @param args  Args required by the syscall.
 */
#define _X_USER(ret, name, args)                    \
	[[gnu::naked]] ret name args                \
	{                                           \
		__asm__("li a7, %[sys_num] \n\t"    \
			"ecall \n\t"                \
			"ret \n\t"                  \
			:                           \
			: [sys_num] "i"(SYS_##name) \
			: "a7");                    \
	}

SYSCALLS(_X_USER)

#undef _X_USER_SYS
