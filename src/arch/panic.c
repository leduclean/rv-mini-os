/**
 * @file
 * @brief Kernel panic: report the call site, then stop the machine.
 */

#include <lib/stdio.h>

#include <arch/cpu.h>

#include <kernel/task.h>

void _panic(const char *msg, const char *file, int line)
{
	irq_disable_s();

	printf("\n[PANIC] %s:%d: %s\n", file, line, msg);

	// NULL before the first task is elected, panic must survive that.
	const task_t *p = task_active();
	if (p) {
		printf("[PANIC] active task: %s (pid %d)\n", p->name,
		       p->pid);
	}

	// Halted, not rebooted: gdb attaches here and the whole state is intact.
	for (;;)
		hlt();
}
