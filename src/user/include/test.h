#pragma once

#include <stdio.h>

/**
 * @brief Prints the test success message.
 */
#define TEST_SUCCESS()                                    \
	do {                                              \
		printf("[TEST] %s SUCCESS \n", __func__); \
	} while (0)

/**
 * @brief Prints the test error message and exits.
 */
#define TEST_ERROR(...)                                               \
	do {                                                          \
		printf("[TEST] %s ERROR %s:%d: ", __func__, __FILE__, \
		       __LINE__);                                     \
		printf(__VA_ARGS__);                                  \
		printf("\n");                                         \
		exit_group(1);                                        \
	} while (0)
