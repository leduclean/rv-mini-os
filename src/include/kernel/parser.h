/**
 * @file
 * @brief Shell line reading and tokenization.
 */

#pragma once

#include "minilib/stddef.h"
#define MAX_ARGS 4 ///< Maximum number of arguments of a shell command.

/** @brief A tokenized shell command line. */
typedef struct {
	int argc; ///< Number of arguments in @p argv.
	char *argv[MAX_ARGS]; ///< Arguments, pointing inside the parsed line.
	int background; ///< 1 if the line ended with an "&", 0 otherwise.
} shell_cmd_tokens_t;

/**
 * @brief Echo the uart chars and write the current line in a buffer.
 *
 * @note Blocks on the uart wait queue until a full line is available.
 * Handles the backspace, and stops on a newline or a carriage return.
 *
 * @param buf Buffer the line is written to, null terminated.
 * @param size Size of @p buf.
 * @return Length of the read line, without its null terminator.
 */
int parser_read_line(char *buf, size_t size);

/**
 * @brief Tokenize a line and convert it into a shell command.
 *
 * @note A trailing "&" argument sets the background flag and is dropped.
 * @warning @p buf is modified in place by the tokenization.
 *
 * @param buf Line to tokenize.
 * @return The tokenized command, with at most MAX_ARGS arguments.
 */
shell_cmd_tokens_t parser_get_cmd(char *buf);
