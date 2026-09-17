/**
 * @file
 * @brief Kernel side syscall dispatch.
 */

#pragma once
#include <stddef.h>
#include <stdint.h>

#include <lib/types.h>

#include <kernel/syscall_def.h>

#define X_DEF_KERNEL(_enum, ret, name, args) ret sys_##name args;
SYSCALLS(X_DEF_KERNEL)
#undef X_DEF_KERNEL

/**
 * @brief Dispatch the call to the correct kernel routine.
 *
 * @param n The syscall number.
 * @param a/b/c/d/e/f syscall args.
 */
long syscall_dispatch(unsigned long n, unsigned long a, unsigned long b,
		      unsigned long c, unsigned long d, unsigned long e,
		      unsigned long f);
