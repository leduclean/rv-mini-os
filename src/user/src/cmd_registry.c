#include "cmd_registry.h"

#include <stddef.h>

#include <kinterface.h>
#include <stdio.h>
#include <string.h>

#include "parser.h"

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

void cmd_help([[maybe_unused]] char *const argv[])
{
	printf("%s", argv[0]);
	for (size_t i = 0; i < REGISTRY_SIZE; i++) {
		const struct cmd *current = &registry[i];
		printf("- %s: %s \r\n  Usage: %s\r\n", current->name,
		       current->desc, current->usage);
	}
}

static int parse_line(char *line, char *argv[], int max_args)
{
	int argc = 0;
	char *token = strtok(line, " ");
	while (token && argc < max_args - 1) {
		argv[argc] = token;
		argc++;
		token = strtok(NULL, " ");
	}
	argv[argc] = NULL;
	return argc;
}

int cmd_get_desc(char *line, struct cmd_desc *desc)
{
	char *argv[EXEC_MAX_ARGS + 1];
	int argc = parse_line(line, argv, EXEC_MAX_ARGS);

	if (argc == 0) {
		return -1;
	}

	char *name = argv[0];
	for (size_t i = 0; i < REGISTRY_SIZE; i++) {
		const struct cmd *current = &registry[i];
		if (strcmp(name, current->name) == 0) {
			desc->f = current->f;
			desc->background = parser_is_bg(&argc, argv);
			memcpy((void *)desc->argv, argv, sizeof(argv));

			return 0;
		}
	}

	return -1;
}
