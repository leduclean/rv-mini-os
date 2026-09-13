/**
 * @file
 * @brief Registry of the commands the shell can run.
 */

#pragma once
#include "process.h"

/** @brief Kind of a registered command. */
typedef enum { CMD_BUILTIN, CMD_PROG } exec_type;

/** @brief A program, spawned in its own process when invoked. */
typedef struct {
	void (*fn)(); ///< Entry point of the program.
	priority prior; ///< Priority the program is spawned with.
} prog_t;

/** @brief A registered command: either a builtin or a spawnable program. */
typedef struct {
	const char *name; ///< Name the command is invoked with.
	exec_type type; ///< Kind of the command, selecting the union member.
	/** @brief Command payload, selected by @p type. */
	union {
		void (*builtin)(); ///< Function called for a CMD_BUILTIN.
		prog_t prog; ///< Program spawned for a CMD_PROG.
	} cmd;
} cmd_desc_t;

/**
 * @brief Register a builtin command, executed in the shell process.
 *
 * @param name Name the command is invoked with.
 * @param fn Function to call.
 * @return 0 on success, -1 on invalid arguments or a full registry.
 */
int8_t cmd_register_builtin(const char *name, void (*fn)());

/**
 * @brief Register a program, spawned in its own process when invoked.
 *
 * @param name Name the program is invoked with.
 * @param fn Entry point of the program.
 * @param prior Priority the program is spawned with.
 * @return 0 on success, -1 on invalid arguments or a full registry.
 */
int8_t cmd_register_prog(const char *name, void (*fn)(), priority prior);

/**
 * @brief Look a command up in the registry.
 *
 * @param name Name to look for.
 * @return Pointer to the matching command, NULL if it is unknown.
 */
const cmd_desc_t *cmd_lookup(const char *name);

/**
 * @brief Get the nth registered command.
 *
 * @param idx Index of the command.
 * @return Pointer to the command, NULL if @p idx is out of range.
 */
const cmd_desc_t *cmd_nth(uint8_t idx);

/**
 * @brief Get the number of registered commands.
 *
 * @return Size of the registry.
 */
uint8_t cmd_count();
