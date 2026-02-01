#pragma once
#include "process.h"
#include <stdint.h>

extern void scheduler_rotate();

extern void scheduler_admit(process_t *proc);

// Sleeping queue functions
extern void wake_up_sleeping();

// Scheduling function
extern void init_scheduler_queues();
extern void scheduler_sleep(uint32_t nbr_secondes);
extern void scheduler_terminate();
extern void proc_launcher(void proc());
