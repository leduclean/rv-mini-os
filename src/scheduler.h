#pragma once
#include <stdint.h>

extern void ordonnance();

// Run queue
extern int enqueue();

// Scheduling function
extern void dors(uint64_t nbr_secondes);
extern void wake_up_sleeping();
extern void fin_processus();
extern void proc_launcher();
