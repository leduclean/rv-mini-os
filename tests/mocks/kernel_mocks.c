#include "process.h"

void scheduler_admit(process_t *p) {}
void scheduler_wake_waiting_queue(wait_queue_t *wq) {}
void init_scheduler_queues() {}

void wq_enqueue(wait_queue_t *wq, clist_node_t *node) {}
void wq_init(wait_queue_t *wq) {}

uint32_t seconds() { return 0; }

uint8_t is_in_sleeping_queue(process_t *p) { return 0; }
void remove_from_sleeping(process_t *p) {}

uint8_t wait_pid(int8_t pid) {}

void hlt() {}

void proc_launcher(void proc()) {}
