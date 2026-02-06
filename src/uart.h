#pragma once
#include "process.h"

void uart_init();
wait_queue_t *uart_get_wait_queue();
void uart_putchar(char);
void uart_irq_handler();
int uart_read(char *c);
