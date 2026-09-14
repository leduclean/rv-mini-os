#include <stdbool.h>
#include <stddef.h>

#include <lib/stdio.h>
#include <lib/tinyalloc.h>

#include <asm/console.h>
#include <asm/cpu.h>
#include <asm/plic.h>
#include <asm/trap.h>
#include <asm/uart.h>

#include <kernel/memory.h>
#include <kernel/mmap.h>
#include <kernel/pages.h>
#include <kernel/process.h>
#include <kernel/shell.h>
#include <kernel/time.h>
#include <user/apps.h>

/** @brief Kernel entry point, called by crt0 once the bss is cleared. */
void kernel_start()
{
	printf("[INFO] Kernel start \n");

	ta_init(_heap_start, _heap_end, TA_HEAP_BLOCK, TA_BLOCK_SPLIT,
		TA_ALIGNMENT);
	printf("[INFO] Tiny alloc initialized \n");

	page_init();
	printf("[INFO] pages initialized \n");

	mmap_kernel();
	printf("[INFO] kernel map \n");

	process_init();
	printf("[INFO] init proc \n");

	irq_enable_s();
	printf("[INFO] S irq enabled\n");

	plic_enable_s_external();

	printf("[INFO] external irq enabled\n");

	if (!process_spawn(shell_run, "shell", NORMAL, false)) {
		printf("[FAILURE]: failed to spawn shell \n");
	}

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
	plic_config_uart();
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
