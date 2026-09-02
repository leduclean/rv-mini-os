#pragma once
#include "minilib/stdint.h"

/**
 * @brief Sleep syscall used to make a process sleep.
 *
 * @param sec Number of sleeping seconds before the process wakes up.
 */
void sleep(uint32_t sec);
