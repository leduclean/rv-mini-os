#pragma once
#ifdef UNIT_TEST

#include "kernel/process/process.h"
#include "kernel/sync/waitqueue.h"
#include "lib/clist.h"

// Mocked functions
void scheduler_admit(process_t *p);
void scheduler_wake_waiting_queue(wait_queue_t *wq);
void init_scheduler_queues(void);

void wq_enqueue(wait_queue_t *wq, clist_node_t *node);
void wq_init(wait_queue_t *wq);

uint32_t seconds(void);

uint8_t is_in_sleeping_queue(process_t *p);
void remove_from_sleeping(process_t *p);

uint8_t wait_pid(int8_t pid);

void hlt(void);

void proc_launcher(void proc());

#endif
