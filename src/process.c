#include "process.h"
#include <scheduler.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define RA_INDEX 0
#define SP_INDEX 1

extern void ctx_sw(uintptr_t old_context_addr, uintptr_t new_context_addr);

/* Create a new processus returning his pid number */
int8_t creer_processus(void code(), char *nom) {
  uint8_t current_pid = proc_table.current_pid;
  if (current_pid >= PROC_TABLE_SIZE) {
    return -1; // Error
  }
  // Get the current process
  process_t *created = &proc_table.table[current_pid];

  // Set the parameter for the created process
  created->pid = current_pid;
  // Initiate the name
  strncpy(created->name, nom, MAXNAME - 1);
  if (current_pid == 0) {
    // Idle case
    // no need to keep ra and rs in the ctx
    created->state = ELECTED;
  } else {
    // Other process is by default activable
    created->state = ACTIVABLE;
    // We need to save the stack pointer and the ctx
    created->ctx[SP_INDEX] = (uint64_t)&created->stack[STACK_SIZE - 1];
    created->ctx[RA_INDEX] = (uintptr_t)code;
  }
  proc_table.current_pid++;
  return current_pid;
}

/* Init processus handling */
void init_proc() {
  uint8_t init_pid = creer_processus(idle, "idle");
  actif = &proc_table.table[init_pid];
}

/** IDLE Processus declaration  **/
void idle() {
  for (;;) {
    printf("[%s] pid = %i\n", mon_nom(), mon_pid());
    ordonnance();
  }
}
