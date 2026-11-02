#pragma once

#include <stdio.h>
#include <string.h>

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

/**
 * @brief Fails the test if actual != expected.
 */
#define TEST_ASSERT_EQ(expected, actual)                                       \
	do {                                                                   \
		long _exp = (expected);                                        \
		long _act = (actual);                                          \
		if (_exp != _act) {                                            \
			TEST_ERROR("%s: expected %ld, got %ld", #actual, _exp, \
				   _act);                                      \
		}                                                              \
	} while (0)

/**
 * @brief Fails the test if actual != expected.
 */
#define TEST_ASSERT_EQ_STR(expected, actual)                                 \
	do {                                                                 \
		const char *_exp = (expected);                               \
		const char *_act = (actual);                                 \
		if (strcmp(_exp, _act) != 0) {                               \
			TEST_ERROR("%s: expected %s, got %s", #actual, _exp, \
				   _act);                                    \
		}                                                            \
	} while (0)
