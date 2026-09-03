#include "console.h"
#include "cpu.h"
#include "irq.h"
#include "minilib/stddef.h"
#include "minilib/stdio.h"
#include "shell.h"
#include "process.h"
#include "progs.h"
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
 * @brief This function will be executed first by the bootloader.
 */
void start()
{
	// Need machine mode privilege (device dependant)
	// Plic config
	init_screen();
	plic_uart_config();
	uart_init();
	pmp_allow_all();

	delegate_traps();

	// Interupt handling
	enable_m_irq();
	enable_m_external();
	//NOTE: Since the timer is clint dependant
	// we don't set up it until timer is redirected
	// to S mode.
	//
	//  A good way could be to trigger a software s interrupt
	//  on each machine interrupt.
	// enable_m_timer();

	init_trap_entries();
	enter_kernel(kernel_start);
}
