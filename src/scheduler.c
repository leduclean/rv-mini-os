
#include "process.h"
#include <stdint.h>

extern void ctx_sw(uintptr_t old_ctx, uintptr_t new_ctx);

/* Return active pid */
int8_t mon_pid() { return actif->pid; }

/* Return the active name */
char *mon_nom() { return actif->name; }

/* Choose the next process to switch on considering the active one */
void ordonnance() {
  int64_t new_pid = mon_pid() + 1;
  if (new_pid >= proc_table.current_pid) {
    // The next pid is going to be idle in this case
    // (no upper process created )
    new_pid = 0;
  }
  process_t *new = &proc_table.table[new_pid];
  actif->state = ACTIVABLE;
  new->state = ELECTED;
  uintptr_t current_ctx = (uintptr_t)&actif->ctx;
  actif = new;
  ctx_sw(current_ctx, (uintptr_t)&new->ctx);
}
