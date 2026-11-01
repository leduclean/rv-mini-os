#pragma once

/**
 * @brief Prints the test success message.
 */
#define TEST_SUCCESS()                                                   \
	do {                                                             \
		write(STDOUT, "[TEST] ", 7);                             \
		write(STDOUT, __func__, __builtin_strlen(__func__));     \
		write(STDOUT, " SUCCESS \n", sizeof(" SUCCESS \n") - 1); \
	} while (0)

/**
 * @brief Prints the test error message.
 */
#define TEST_ERROR()                                                 \
	do {                                                         \
		write(STDOUT, "[TEST] ", 7);                         \
		write(STDOUT, __func__, __builtin_strlen(__func__)); \
		write(STDOUT, " ERROR \n", sizeof(" ERROR \n") - 1); \
		exit_group(1);                                       \
	} while (0)
