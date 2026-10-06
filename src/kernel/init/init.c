#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lib/stdio.h>
#include <lib/tinyalloc.h>

#include <asm/cpu.h>
#include <asm/trap.h>

#include <drivers/console.h>
#include <drivers/plic.h>
#include <drivers/uart.h>
#include <drivers/virtio_blk.h>

#include <kernel/memory.h>
#include <kernel/mmap.h>
#include <kernel/pages.h>
#include <kernel/process.h>
#include <kernel/shell.h>
#include <kernel/time.h>

/** @brief Kernel entry point, called by crt0 once the bss is cleared. */

[[noreturn]] void kernel_start(void)
{
	int res;
	res = mmap_kernel();
	if (res < 0) {
		panic("[FAILURE] Kernel map has failed");
	}

	// PCI device init
	res = console_init();
	if (res < 0) {
		panic("[FAILURE] Console init map has failed");
	}

	res = virtio_blk_init();
	if (res < 0) {
		panic("[FAILURE] Virtio blk driver init has failed");
	}

	ta_init(_heap_start, _heap_end, TA_HEAP_BLOCK, TA_BLOCK_SPLIT,
		TA_ALIGNMENT);
	printf("[INFO] Tiny alloc initialized \n");

	printf("[INFO] init proc \n");
	process_init();
	if (res < 0) {
		panic("[FAILURE] Init proc has failed");
	}

	printf("[INFO] S irq enabled\n");
	irq_enable_s();

	printf("[INFO] external irq enabled\n");
	plic_init();

	res = time_spawn_daemons();
	if (res < 0) {
		panic("[FAILURE] Time daemon spawn has failed");
	}

	res = uart_spawn_daemons();
	if (res < 0) {
		panic("[FAILURE] Uart daemon spawn has failed");
	}

	if (!process_spawn(shell_run, "shell", NORMAL, false)) {
		printf("[FAILURE]: failed to spawn shell \n");
	}

	process_idle();
}

extern void enter_kernel(void (*entry)(void));
extern void delegate_traps(void);

/**
 * @brief Boot entry point in machine mode.
 */
void start(void)
{
	page_init();
	//TODO: Maybe it should resides in S mode
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
