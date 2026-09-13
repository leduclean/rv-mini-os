#include "console.h"
#include "cpu.h"
#include "irq.h"
#include "minilib/stddef.h"
#include "minilib/stdio.h"
#include "minilib/stdbool.h"
#include "mmap.h"
#include "pages.h"
#include "shell.h"
#include "process.h"
#include "apps.h"
#include "time.h"
#include "tinyalloc.h"
#include "trap.h"
#include "uart.h"
#include "memory.h"

/** @brief Kernel entry point, called by crt0 once the bss is cleared. */
void kernel_start()
{
	printf("[INFO] Kernel start \n");

	ta_init(_heap_start, _heap_end, TA_HEAP_BLOCK, TA_BLOCK_SPLIT,
		TA_ALIGNMENT);
	printf("[INFO] Tiny alloc initialized \n");

	pages_init();
	printf("[INFO] pages initialized \n");

	mmap_kernel();
	printf("[INFO] kernel map \n");

	process_init();
	printf("[INFO] init proc \n");

	enable_s_irq();
	printf("[INFO] S irq enabled\n");

	enable_s_external();

	printf("[INFO] external irq enabled\n");
	if (!process_spawn(segfault_test, "test", NORMAL, true)) {
		printf("[FAILURE]: failed to spawn test");
	}

	// process_spawn(shell, "shell", NORMAL, false);
	// printf("[INFO]: spawned shell");
	process_idle();
}

extern void enter_kernel(void (*entry)());
extern void delegate_traps();

/**
 * @brief Boot entry point in machine mode.
 */
void start()
{
	// Device init
	console_init();
	plic_uart_config();
	uart_init();
	pmp_allow_all();

	delegate_traps();
	trap_init();

	// Timer init.
	// WARNING: this should always resides
	// just before entering the kernel and S mode.
	time_init();

	enter_kernel(kernel_start);
}
