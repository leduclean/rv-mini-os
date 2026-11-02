#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include <lib/err.h>
#include <lib/stdio.h>
#include <lib/tinyalloc.h>

#include <arch/cpu.h>
#include <arch/trap.h>

#include <drivers/console.h>
#include <arch/plic.h>
#include <drivers/uart.h>
#include <drivers/virtio_blk.h>

#include <kernel/fs.h>
#include <kernel/memory.h>
#include <kernel/mmap.h>
#include <kernel/pages.h>
#include <kernel/process.h>
#include <kernel/time.h>

/**
 * @brief User elf included via user_app.S.
 */
extern char _user_elf_start[];

/**
 * @brief Handle all the blocking initialization.
 */
static void init(void)
{
	int res;
	/* Blocking task that require blk io */
	res = fs_init();
	if (res < 0) {
		panic("[FAILURE] File system init has failed");
	}

	res = process_map_console(process_active());
	if (res < 0) {
		panic("[FAILURE] Console map has failed");
	}

	process_t *p = process_start_userland((uintptr_t)_user_elf_start,
					      "User", NORMAL);
	if (IS_ERR(p)) {
		printf("[FAILURE]: failed to enter userland (%ld)\n",
		       PTR_ERR(p));
	}
}

/** @brief Kernel entry point, called by crt0 once the bss is cleared. */
[[noreturn]] void kernel_start(void)
{
	int res;
	res = mmap_kernel();
	if (res < 0) {
		panic("[FAILURE] Kernel map has failed");
	}

	res = console_init();
	if (res < 0) {
		panic("[FAILURE] Console init map has failed");
	}

	res = virtio_blk_init();
	if (res < 0) {
		panic("[FAILURE] Virtio blk driver init has failed");
	}

	ta_init(_kheap_start, _kheap_end, TA_HEAP_BLOCK, TA_BLOCK_SPLIT,
		TA_ALIGNMENT);
	printf("[INFO] Tiny alloc initialized \n");

	printf("[INFO] init proc \n");
	process_init();
	if (res < 0) {

		panic("[FAILURE] Init proc has failed");
	}

	/*WARNING: from here we are in the idle process.
     * This process cannot sleep so if a task as io, use init() 
     * task to do it. */
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

	if (IS_ERR(process_kthread(init, "init", NORMAL))) {
		printf("[FAILURE]: failed to spawn init kthread \n");
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
