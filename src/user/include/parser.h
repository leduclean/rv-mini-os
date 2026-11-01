/**
 * @file
 * @brief Shell line reading and tokenization.
 */

#pragma once

#include <stddef.h>

#include <kinterface.h>

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
ssize_t parser_scan_line(char *line, size_t max_size);

/**
 * @brief If the last cmd argument is &, set background and drop that arg.
 *
 * @param argc Number of values of the command.
 * @param argv values of the command to check.
 */
bool parser_is_bg(int *argc, char *argv[]);
