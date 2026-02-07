#pragma once
#include "circ_queue.h"
#include "process.h"
#include <stdint.h>

extern void scheduler_rotate();

extern void scheduler_admit(process_t *proc);

void scheduler_ready_process(process_t *proc);

// Sleeping queue functions
extern void scheduler_wake_sleeping();
void scheduler_wake_waiting_queue(wait_queue_t *wq);
void remove_from_sleeping(process_t *proc);
uint8_t is_in_sleeping_queue(process_t *proc);

// Waiting queue handling
void scheduler_block_on(wait_queue_t *wq);
void scheduler_block_on_with_timeout(wait_queue_t *wq, uint32_t timeout_secs);

// Scheduling function
extern void init_scheduler_queues();
extern void scheduler_sleep(uint32_t nbr_secondes);
extern void scheduler_terminate();
extern void proc_launcher(void proc());
