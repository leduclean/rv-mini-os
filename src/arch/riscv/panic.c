/**
 * @file
 * @brief Kernel panic: report the call site, then stop the machine.
 */

#include "cpu.h"
#include "process.h"
#include "minilib/stdio.h"

void _panic(const char *msg, const char *file, int line)
{
	// Freeze first: whatever we are about to print must not move under us,
	// and a timer irq here would reschedule out of a dead kernel.
	disable_s_irq();
	disable_m_irq();

	printf("\n[PANIC] %s:%d: %s\n", file, line, msg);

	// NULL before the first process is elected, panic must survive that.
	const process_t *p = get_active();
	if (p) {
		printf("[PANIC] active process: %s (pid %d)\n", p->name,
		       p->pid);
	}

	// Halted, not rebooted: gdb attaches here and the whole state is intact.
	for (;;)
		hlt();
}
