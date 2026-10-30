#pragma once

/**
 * @brief Sleep user system call test.
 */
void sleep_call(void);

/**
 * @brief Fork app that test the copy of the page table with CoW.
 */
void fork_test(void);

/**
 * @brief Exec app that test the exec of the sleep_call app.
 */
void exec_test(void);

/**
 * @brief Small workaround app with fork and exec syscalls.
 */
void fork_exec_test(void);

/**
 * @brief Test the segfault handling.
 */
void segfault_test(void);

/**
 * @brief Read write roundtrip.
 */
void fs_io_test(void);

/**
 * @brief Test the read and write syscall by streaming input in the console.
 */
void stream_test(void);

/**
 * @brief Test the thread api.
 */
void thread_test(void);
