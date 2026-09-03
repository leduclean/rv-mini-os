#include "console.h"
#include "cpu.h"
#include "irq.h"
#include "minilib/stddef.h"
#include "minilib/stdio.h"
#include "shell.h"
#include "process.h"
#include "progs.h"
#include "time.h"
#include "trap.h"
#include "uart.h"
#include "memory.h"

static long ustack1[KSTACK_SIZE] = { 0 };
static long ustack2[KSTACK_SIZE] = { 0 };

void u1()
{
	enter_user_mode(sleep_call, (uintptr_t)&ustack1[KSTACK_SIZE]);
}

void u2()
{
	enter_user_mode(sleep_call, (uintptr_t)&ustack2[KSTACK_SIZE]);
}

/** @brief Kernel entry point, called by crt0 once the bss is cleared. */
void kernel_start()
{
	enable_s_irq();
	enable_s_external();

	init_proc();
	spawn_process(shell, "shell", NORMAL);
	idle();
}

extern void enter_kernel(void (*entry)());
extern void delegate_traps();

/**
 * @brief Boot entry point in machine mode.
 */
void start()
{
	// Device init
	init_screen();
	plic_uart_config();
	uart_init();
	pmp_allow_all();

	delegate_traps();
	init_trap_entries();

	// Timer init.
	// WARNING: this should always resides
	// just befor entering the kernel and S mode.
	init_timer();

	enter_kernel(kernel_start);
}
