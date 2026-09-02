#include "console.h"
#include "cpu.h"
#include "irq.h"
#include "minilib/stddef.h"
#include "process.h"
#include "progs.h"
#include "trap.h"
#include "uart.h"
#include "memory.h"

extern void trap_entry(void);

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
	// Plic config
	plic_uart_config();
	init_proc();
	init_screen();
	uart_init();

	// Config the user mode
	pmp_allow_all();

	// Interupt handling
	_enable_it();
	init_trap_entry(trap_entry);
	enable_external();
	enable_timer();

	spawn_process(u1, "u1", NORMAL);
	spawn_process(u2, "u2", NORMAL);
	idle();
}
