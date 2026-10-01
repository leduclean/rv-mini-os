#pragma once
#include <stdint.h>

#include <user/syscall.h>

// We just copy the clone flags for now
#define CLONE_VM (1 << 0)

// This is statically defined for now
typedef struct thread_entry {
	void (*fn)(void *args);
	void *args;
} thread_entry_t;

// Takes stack for now
uint8_t thread_create(thread_entry_t *thread_entry, void *stack);

int thread_join(uint8_t tid);
