#pragma once
#include "process.h"
#include "minilib/stdint.h"

void scheduler_rotate();

void scheduler_admit(process_t *proc);

void scheduler_ready_process(process_t *proc);

// Sleeping queue functions
void scheduler_wake_sleeping();
void scheduler_wake_waiting_queue(wait_queue_t *wq);
void remove_from_sleeping(process_t *proc);
uint8_t is_in_sleeping_queue(process_t *proc);

// Waiting queue handling
void scheduler_block_on(wait_queue_t *wq);
void scheduler_block_on_with_timeout(wait_queue_t *wq, uint32_t timeout_secs);

// Scheduling function
void init_scheduler_queues();
void scheduler_sleep(uint32_t nbr_secondes);
void scheduler_terminate();
void proc_launcher(void proc());
