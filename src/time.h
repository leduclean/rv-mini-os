#pragma once
#include <stdint.h>

// Timer irq enable/disable
extern void enable_timer();
extern void disable_timer();

extern uint32_t nbr_secondes();
extern void init_traitant(void traitant());
