#include "process.h"
#include "time.h"
#include <stddef.h>
#include <stdint.h>

extern void ctx_sw(uintptr_t old_ctx, uintptr_t new_ctx);

/* Return active pid */
uint8_t mon_pid() { return actif->pid; }

/* Return the active name */
char *mon_nom() { return actif->name; }

/* Try to wake up a process */
void try_wake_up(process_t *proc) {
  // Wake up if new
  if (proc->wake_up_time <= nbr_secondes()) {
    proc->state = READY;
  }
}

process_t *find_next() {
  uint8_t pid = mon_pid();
  process_t *new = NULL;
  for (int8_t scanned = 0; scanned < PROC_TABLE_SIZE; scanned++) {
    pid = (pid + 1) % PROC_TABLE_SIZE;
    process_t *candidate = &proc_table.table[pid];
    if (candidate->state == TERMINATED || (candidate->state == FREE)) {
      continue;
    };

    if (candidate->state == SLEEPING) {
      try_wake_up(candidate);
    }

    if (candidate->state == READY) {
      new = candidate;
      break;
    }
  }
  return new;
}

/* Choose the next process to switch on considering the active one */
void ordonnance() {
  process_t *next = find_next();
  // fallback sur idle si rien n’est READY
  if (!next) {
    next = &proc_table.table[0]; // idle
  }
  if (actif->state == RUNNING) {
    // Only a RUNNING processus comeback to ready
    actif->state = READY;
  }
  next->state = RUNNING;
  uintptr_t current_ctx = (uintptr_t)&actif->ctx;
  actif = next;
  ctx_sw(current_ctx, (uintptr_t)&next->ctx);
}
