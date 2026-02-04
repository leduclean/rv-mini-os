#pragma once
#include <stdint.h>

// irq triggered functions
void init_trap_entry(void (*entry)());
void trap_handler(uint64_t mcause, uint64_t mie, uint64_t mip);

// Timer interrupts
void disable_timer();
void enable_timer();
