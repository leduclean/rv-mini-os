#include "include/mmio.h"
#include "platform.h"
#include "process.h"
#include "time.h"
#include <stddef.h>
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
  int64_t pid = mon_pid();
  process_t *new = NULL;
  for (uint8_t scanned = 0; scanned < proc_table.current_pid; scanned++) {
    pid = (pid + 1) % proc_table.current_pid;
    process_t *candidate = &proc_table.table[pid];

    if (candidate->state == TERMINATED)
      continue; // Skip terminated processes explicitly

    if (candidate->state == SLEEPING) {
      try_wake_up(candidate);
    }

    // Choose if READY -> either waked up
    // or already activable
    if (candidate->state == READY) {
      new = candidate;
      break;
    }
  }
  // fallback sur idle si rien n’est READY
  if (!new) {
    new = &proc_table.table[0]; // idle
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
