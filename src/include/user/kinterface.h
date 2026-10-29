/**
 * @file
 * @brief This file declare the common interface between the kernel and the user.
 */

#pragma once

/** 
* @name Open syscall flags.
* @{*/
#define O_MODE_MASK (0b111)
#define O_R (1UL << 0)
#define O_W (1UL << 1)
#define O_X (1UL << 2)
#define O_CREAT (1UL << 3)
/** @}*/

#define STDOUT 1 /* Console file descriptor number */
#define STDIN 2 /* Console input file descriptor number */
