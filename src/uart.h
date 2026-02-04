#pragma once

void uart_init();

void uart_putchar(char);
void uart_irq_handler();
int uart_read(char *c);
