#pragma once
#include <stdint.h>

// Timer irq enable/disable
extern void enable_timer();
extern void disable_timer();

extern void trap_handler(uint64_t mcause, uint64_t mie, uint64_t mip);
extern uint32_t seconds();
extern void init_traitant(void traitant());
