#pragma once

/**
 * @brief Sleep user system call test.
 */
void sleep_call();

/**
 * @brief Fork app that test the copy of the page table with CoW.
 */
void fork_test();

/**
 * @brief Test the segfault handling.
 */
void segfault_test();
