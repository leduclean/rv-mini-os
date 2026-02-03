#include "process.h"
#include "scheduler.h"
#include "time.h"
#include <stdint.h>

/** We prefer an english semantic for uniformity
 * and code clarity but we need aliases for CI eval **/

int8_t cree_processus(void (*entry)(void), char *name) {
  return spawn_process(entry, name);
}

void ordonnance(void) { scheduler_rotate(); }

void dors(uint32_t sec) { scheduler_sleep(sec); }

void fin_processus(void) { scheduler_terminate(); }

uint8_t mon_pid(void) { return get_active_pid(); }

char *mon_nom(void) { return get_active_name(); }

uint32_t nbr_secondes(void) { return seconds(); }
