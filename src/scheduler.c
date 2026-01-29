#include "include/mmio.h"
#include "platform.h"
#include "process.h"
#include "time.h"
#include <stdint.h>

extern void ctx_sw(uintptr_t old_ctx, uintptr_t new_ctx);

/* Return active pid */
int8_t mon_pid() { return actif->pid; }

/* Return the active name */
char *mon_nom() { return actif->name; }

/* Try to wake up a process */
void try_wake_up(process_t *proc) {
  // Wake up if new
  if (proc->wake_up_time <= nbr_secondes()) {
    proc->state = READY;
  }
}
/* Choose the next process to switch on considering the active one */
void ordonnance() {
  int64_t new_pid = mon_pid() + 1;
  process_t *new;
  for (uint8_t scanned = 0; scanned < proc_table.current_pid; scanned++) {
    if (new_pid >= proc_table.current_pid) {
      // The next pid is going to be idle in this case
      // (no upper process created )
      new_pid = 0;
    }
    new = &proc_table.table[new_pid];

    if (new->state == SLEEPING) {
      try_wake_up(new);
    }

    // Choose if WAITING -> either waked up
    // or already activable
    if (new->state == READY)
      break;
    new_pid++;
  }
  if (actif->state == RUNNING) {
    // Only a RUNNING processus comeback to ready
    actif->state = READY;
  }
  new->state = RUNNING;
  uintptr_t current_ctx = (uintptr_t)&actif->ctx;
  actif = new;
  ctx_sw(current_ctx, (uintptr_t)&new->ctx);
}
