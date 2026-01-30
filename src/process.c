#include "process.h"
#include "cpu.h"
#include "time.h"
#include <scheduler.h>
#include <stdint.h>
#include <string.h>

#define RA_INDEX 0
#define SP_INDEX 1
#define S0_INDEX 4

extern void ctx_sw(uintptr_t old_context_addr, uintptr_t new_context_addr);

/** Terminate a processus **/
void fin_processus() {
  actif->state = TERMINATED;
  proc_table.active_process--;
  ordonnance();
}

/* Launcher to handle launch and terminaison of a proc */
void proc_launcher(void proc()) {
  proc();
  fin_processus();
}

/* Processus Table setter */
int8_t set_process(void code(), char *nom, process_t *slot) {
  // Set the parameter for the created process
  slot->pid = proc_table.next_pid;
  // Initiate the name
  strncpy(slot->name, nom, MAXNAME - 1);
  slot->state = READY;
  if (slot == &proc_table.table[0]) {
    slot->ctx[RA_INDEX] = (uintptr_t)idle;
  } else {
    // We need to save the stack pointer and the ctx
    slot->ctx[SP_INDEX] = (uint64_t)&slot->stack[STACK_SIZE - 1];
    slot->ctx[RA_INDEX] = (uintptr_t)proc_launcher;
    slot->ctx[S0_INDEX] = (uintptr_t)code;
  }
  proc_table.active_process++;
  proc_table.next_pid++;
  return slot->pid;
}

/** Find an empty slot for a processus **/
process_t *find_slot() {
  process_t *candidate = NULL;
  for (uint8_t slot_idx = 0; slot_idx < PROC_TABLE_SIZE; slot_idx++) {
    candidate = &proc_table.table[slot_idx];
    if (candidate->state == FREE || candidate->state == TERMINATED) {
      break;
    }
  }
  return candidate;
}

/* Create a new processus returning his pid number */
int8_t creer_processus(void code(), char *nom) {
  if (proc_table.active_process >= PROC_TABLE_SIZE) {
    return -1; // Error already max processus launched
  }
  process_t *slot = find_slot();
  if (!slot)
    return -1;
  return set_process(code, nom, slot);
}

/* Init processus handling */
void init_proc() {
  uint8_t init_pid = creer_processus(idle, "idle");
  actif = &proc_table.table[init_pid];
  actif->state = RUNNING;
}

/** IDLE Processus declaration  **/
void idle() {
  for (;;) {
    enable_it();
    hlt();
    disable_it();
  }
}

/** Set a program to sleeping state **/
void dors(uint64_t nbr_secs) {
  actif->wake_up_time = nbr_secs + nbr_secondes();
  actif->state = SLEEPING;
  ordonnance();
}
