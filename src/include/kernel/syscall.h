/**
 * @file
 * @brief Kernel side syscall dispatch.
 */

#pragma once

/**
 * @brief Dispatch the call to the correct kernel routine.
 *
 * @param n SYS number.
 * @param a First param for the routine.
 * @param b Second param for the routine.
 * @param c Third param for the routine.
 */
long syscall_dispatch(long n, long a, long b, long c);
