/**
 * @file
 * @brief Interactive shell.
 */

#pragma once

/**
 * @brief Shell main process.
 *
 * @note Registers the builtins and the programs, then loops reading and
 * running the user commands. Never returns.
 */
void shell_run();
