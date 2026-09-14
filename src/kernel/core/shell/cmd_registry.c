#include "cmd_registry.h"
#include <stddef.h>
#include <string.h>
#include "process.h"

#define MAX_CMDS 64
/** @brief Fixed size table of the registered commands. */
typedef struct {
	cmd_desc_t tab[MAX_CMDS]; ///< Registered commands, in insertion order.
	uint8_t size; ///< Number of registered commands.
} exec_table;

static exec_table registry;

int8_t cmd_register_builtin(const char *name, void (*fn)())
{
	if (!name || !fn || registry.size >= MAX_CMDS)
		return -1;
	cmd_desc_t *tab = registry.tab;
	uint8_t size = registry.size;
	tab[size].name = name;
	tab[size].type = CMD_BUILTIN;
	tab[size].cmd.builtin = fn;
	registry.size++;
	return 0;
}

int8_t cmd_register_prog(const char *name, void (*fn)(), priority prior,
			 bool user)
{
	if (!name || !fn || registry.size >= MAX_CMDS)
		return -1;
	cmd_desc_t *tab = registry.tab;
	uint8_t size = registry.size;
	tab[size].name = name;
	tab[size].type = CMD_PROG;
	tab[size].cmd.prog.fn = fn;
	tab[size].cmd.prog.prior = prior;
	tab[size].cmd.prog.user = user;
	registry.size++;
	return 0;
}

const cmd_desc_t *cmd_lookup(const char *name)
{
	if (!name)
		return NULL;
	for (size_t i = 0; i < registry.size; ++i) {
		if (registry.tab[i].name &&
		    strcmp(registry.tab[i].name, name) == 0)
			return &registry.tab[i];
	}
	return NULL;
}

const cmd_desc_t *cmd_nth(uint8_t idx)
{
	if (idx >= registry.size) {
		return NULL;
	}
	return &registry.tab[idx];
}

uint8_t cmd_count()
{
	return registry.size;
}
