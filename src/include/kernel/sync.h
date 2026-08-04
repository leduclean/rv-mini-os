/**
 * @file
 * @brief Parent side synchronisation primitives.
 */

#pragma once
#include "minilib/stdint.h"

/**
 * @brief Wait for the terminaison of any child, then reap it.
 *
 * @note Blocks the caller until one of its children becomes a zombie.
 *
 * @return Pid of the reaped child.
 */
uint8_t wait();

/**
 * @brief Wait for the terminaison of a specific child, then reap it.
 *
 * @note Blocks the caller until this child becomes a zombie.
 *
 * @param pid Pid of the child to wait for.
 * @return Pid of the reaped child.
 */
uint8_t wait_pid(int8_t pid);
