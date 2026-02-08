#pragma once

#include "lib/stdint.h"
typedef struct semaphore semaphore_t;

void sem_init(semaphore_t *sem, int val);
void sem_release(semaphore_t *sem);
int8_t sem_try_take(semaphore_t *sem);
void sem_take(semaphore_t *sem);
