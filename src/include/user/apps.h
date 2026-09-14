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

/**
 * @brief Test the write syscall.
 */
void write_test();

/**
 * @brief Test the read and write syscall by reading then writing 
 * in the console.
 */
void read_test();

/**
 * @brief Test the read and write syscall by streaming input in the console.
 */
void stream_test();
