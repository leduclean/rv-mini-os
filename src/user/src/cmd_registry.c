#include "cmd_registry.h"

#include <stddef.h>

#include <err.h>
#include <kinterface.h>
#include <stdio.h>
#include <string.h>

#define MAX_CMDS 64

struct cmd {
	cmd_func_t f;
	const char *name;
	const char *desc;
	const char *usage;
};

#define X_ENTRY(n, d, u) { .f = cmd_##n, .name = #n, .desc = d, .usage = u },
/**
 * @brief Register of all the @p struct cmd declared in the CMD_LIST.
 */
static const struct cmd registry[] = { CMD_LIST(X_ENTRY) };
#undef X_ENTRY

#define REGISTRY_SIZE (sizeof(registry) / sizeof(registry[0]))

void cmd_help([[maybe_unused]] int argc, [[maybe_unused]] char *const argv[])
{
	printf("%s", argv[0]);
	for (size_t i = 0; i < REGISTRY_SIZE; i++) {
		const struct cmd *current = &registry[i];
		printf("- %s: %s \r\n  Usage: %s\r\n", current->name,
		       current->desc, current->usage);
	}
}

cmd_func_t cmd_find(char *const name)
{
	for (size_t i = 0; i < REGISTRY_SIZE; i++) {
		const struct cmd *current = &registry[i];
		if (strcmp(name, current->name) == 0) {
			return current->f;
		}
	}

	return NULL;
}
