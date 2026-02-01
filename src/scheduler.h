#pragma once
#include "process.h"
#include <stdint.h>

extern void scheduler_rotate();

// Run Queue
typedef struct {
  process_t *queue[MAX_PROC];
  uint8_t head; // Idx active
  uint8_t tail; // Idx for next insertion
  uint8_t size; // number of active element
} circ_queu_t;

extern int enqueue(process_t *proc);

// Sleeping queue functions
extern void wake_up_sleeping();

// Scheduling function
extern void scheduler_sleep(uint32_t nbr_secondes);
extern void scheduler_terminate();
extern void proc_launcher(void proc());
