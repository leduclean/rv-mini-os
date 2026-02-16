#pragma once
#include "process.h"
#include "minilib/stdint.h"

typedef struct mutex mutex_t;

void mutex_init(mutex_t *m);
uint8_t mutex_is_lock(mutex_t *m);
process_t *mutex_owner(mutex_t *m);
void mutex_lock(mutex_t *m);
int8_t mutex_trylock(mutex_t *m);
void mutex_trylock_timeout(mutex_t *m);
void mutex_unlock(mutex_t *m);
