#include "arch/riscv/irq.h"
#include "kernel/process/process.h"
#include "kernel/sched/scheduler.h"
#include "kernel/time/time.h"
#include "drivers/uart.h"
#include "lib/minilib/stdint.h"

/** We prefer an english semantic for uniformity
 * and code clarity but we need aliases for CI eval **/

void init_uart() { return uart_init(); }
int8_t cree_processus(void (*entry)(void), char *name, priority prior) {
  spawn_process(entry, name, prior);
  return get_active_pid();
}

void ordonnance(void) { scheduler_rotate(); }

void dors(uint32_t sec) { scheduler_sleep(sec); }

void fin_processus(void) { scheduler_terminate(); }

uint8_t mon_pid(void) { return get_active_pid(); }

char *mon_nom(void) { return get_active_name(); }

uint32_t nbr_secondes(void) { return seconds(); }

void init_traitant(void (*traitant)(void)) { return init_trap_entry(traitant); }
