/**
 * @file
 * @brief Kernel panic: report the call site, then stop the machine.
 */

#include <stdio.h>

#include "cpu.h"
#include "process.h"

void _panic(const char *msg, const char *file, int line)
{
	irq_disable_s();

	printf("\n[PANIC] %s:%d: %s\n", file, line, msg);

	// NULL before the first process is elected, panic must survive that.
	const process_t *p = process_active();
	if (p) {
		printf("[PANIC] active process: %s (pid %d)\n", p->name,
		       p->pid);
	}

	// Halted, not rebooted: gdb attaches here and the whole state is intact.
	for (;;)
		hlt();
}
