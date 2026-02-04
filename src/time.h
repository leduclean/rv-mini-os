#pragma once
#include <stdint.h>

uint32_t seconds();
void init_timer();

void timer_irq_handler(void);
