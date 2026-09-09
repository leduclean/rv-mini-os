#pragma once
#include "minilib/stdint.h"

/**
 * @brief Sleep syscall used to make a process sleep.
 *
 * @param sec Number of sleeping seconds before the process wakes up.
 */
void sleep(uint32_t sec);

/**
 * @brief Exit syscall called on a user process terminaison.
 *
 * This function cleans the entire struct associated with the process itself.
 *
 * @param code Terminaison code: 0 if normal, else error codes.
 */
void exit(int code);
