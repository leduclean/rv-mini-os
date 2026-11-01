/**
 * @file
 * @brief This file declare the common interface between the kernel and the user.
 */

#pragma once

/*
 * size_t and ptrdiff_t come from the compiler's freestanding <stddef.h>.
 * ssize_t is POSIX, so GCC does not provide it: it is the signed counterpart
 * of size_t, which __PTRDIFF_TYPE__ gives us on every sane target.
 */
typedef __PTRDIFF_TYPE__ ssize_t;

/** 
* @name Open syscall flags.
* @{*/
#define O_MODE_MASK (0b111)
#define O_R (1UL << 0)
#define O_W (1UL << 1)
#define O_X (1UL << 2)
#define O_CREAT (1UL << 3)
/** @}*/

#define STDOUT 0 /* Console file descriptor number. */
#define STDIN 1 /* Console input file descriptor number. */

/**
 * @name Clone syscall flags.
 * @{
 */
#define CLONE_VM (1 << 0) /**< Caller and child run in the same memory. */
#define CLONE_FILES \
	(1 << 1) /**< Caller and child share the same file descriptors. */
/** @} */

/**
 * @name Seek whence mode for the lseek() syscall.
 * @{
 */
#define SEEK_SET (1 << 0) /**< File offset set to offset bytes. */
#define SEEK_CUR (1 << 1) /**< File offset set to its location + offset. */
#define SEEK_END (1 << 2) /**< File offset set to size of the file + offset. */
/** @} */
