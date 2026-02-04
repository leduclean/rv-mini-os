#pragma once
#include "circ_queue.h"

void uart_init();
circ_queue_t *uart_get_wait_queue();
void uart_putchar(char);
void uart_irq_handler();
int uart_read(char *c);
