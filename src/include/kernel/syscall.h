/**
 * @file
 * @brief Kernel side syscall dispatch.
 */

#pragma once
#include <stddef.h>
#include <stdint.h>

#include <lib/types.h>

/**
 * @brief Dispatch the call to the correct kernel routine.
 *
 * @param n SYS number.
 * @param a First param for the routine.
 * @param b Second param for the routine.
 * @param c Third param for the routine.
 */
long syscall_dispatch(long n, long a, long b, long c);

void sys_sleep(uint32_t s);

int8_t sys_wait(void);
int8_t sys_wait_pid(int8_t pid);

void sys_exit(int8_t exit_code);
int8_t sys_get_pid(void);
int8_t sys_fork(void);

ssize_t sys_write(int fd, char *buf, size_t count);
ssize_t sys_read(int fd, char *buf, size_t count);
