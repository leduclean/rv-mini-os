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

/**
 * @brief Wait for the terminaison of any child, then reap it.
 *
 * @note Blocks the caller until one of its children becomes a zombie.
 *
 * @return Pid of the reaped child.
 */
int8_t wait();

/**
 * @brief Wait for the terminaison of a specific child, then reap it.
 *
 * @note Blocks the caller until this child becomes a zombie.
 *
 * @param pid Pid of the child to wait for.
 * @return Pid of the reaped child.
 */
int8_t wait_pid(int8_t pid);

/**
 * @brief Dupplicate a process state making a copy of it execution
 *
 * @return The pid of the child process in the parent flow or 0 
 * in the child flow.
 */
int8_t fork();
