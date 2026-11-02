#include <stddef.h>

#include "cmd_registry.h"
#include "err.h"
#include "shell.h"

int main(int argc, char *const argv[])
{
	if (argc == 0) {
		sh();
	} else {
		cmd_func_t f = cmd_find(argv[0]);
		if (!f) {
			return -ENOENT;
		}

		f(argc, argv);
	}

	return 0;
}
