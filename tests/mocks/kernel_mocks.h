#pragma once
#ifdef UNIT_TEST

#include "clist.h"
#include "process.h"
#include "waitqueue.h"

// Mocked functions
typedef unsigned long irq_flags_t;
irq_flags_t irq_save();
void irq_restore(irq_flags_t state);
void scheduler_admit(process_t *p);
void scheduler_wake_waiting_queue(wait_queue_t *wq);
void scheduler_init(void);

void wq_enqueue(wait_queue_t *wq, clist_node_t *node);
void wq_init(wait_queue_t *wq);

uint32_t seconds(void);

uint8_t scheduler_is_sleeping(process_t *p);
void scheduler_remove_sleeping(process_t *p);

uint8_t wait_pid(int8_t pid);

void hlt(void);

void proc_launcher(void proc());

#endif
