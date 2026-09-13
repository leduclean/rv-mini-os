#include "arch/riscv/irq.h"
#include "kernel/process/process.h"
#include "kernel/sched/scheduler.h"
#include "kernel/time/time.h"
#include "drivers/uart.h"
#include "lib/minilib/stdint.h"

/**
 * @file
 * @brief French aliases of the kernel API, required by the CI eval.
 *
 * We prefer an english semantic for uniformity and code clarity, so the
 * kernel keeps english names and this file only forwards the calls.
 */

/** @brief Alias of uart_init(). */
void init_uart()
{
	return uart_init();
}

/**
 * @brief Alias of process_spawn().
 *
 * @param entry Entry point of the process.
 * @param name Name of the process.
 * @param prior Priority of the process.
 * @return Pid of the active process.
 */
int8_t cree_processus(void (*entry)(void), char *name, priority prior)
{
	process_spawn(entry, name, prior);
	return process_active()->pid;
}

/** @brief Alias of scheduler_rotate(). */
void ordonnance(void)
{
	scheduler_rotate();
}

/**
 * @brief Alias of scheduler_sleep().
 *
 * @param sec Sleeping duration, in secondes.
 */
void dors(uint32_t sec)
{
	scheduler_sleep(sec);
}

/** @brief Alias of scheduler_terminate(). */
void fin_processus(void)
{
	scheduler_terminate();
}

/**
 * @brief Alias of process_active()->pid.
 *
 * @return Pid of the active process.
 */
uint8_t mon_pid(void)
{
	return process_active()->pid;
}

/**
 * @brief Alias of process_active()->name.
 *
 * @return Name of the active process.
 */
char *mon_nom(void)
{
	return process_active()->name;
}

/**
 * @brief Alias of seconds().
 *
 * @return Number of secondes since boot.
 */
uint32_t nbr_secondes(void)
{
	return seconds();
}

/**
 * @brief Alias of init_trap_entry().
 *
 * @param traitant Trap vector to install.
 */
void init_traitant(void (*traitant)(void))
{
	return init_trap_entry(traitant);
}
