/**
 * @file
 * @brief 16550 uart driver, interrupt driven on reception.
 */

#pragma once
#include "waitqueue.h"

/**
 * @brief Init the uart to send and receive chars.
 *
 * @note Sets the baud rate, enables the fifo and the RX interrupt.
 */
void uart_init();

/**
 * @brief Get the queue of the processes blocked on a uart read.
 *
 * @return Pointer to the uart wait queue.
 */
wait_queue_t *uart_get_wait_queue();

/**
 * @brief Write a character in the THR fifo buffer, to transmit it.
 *
 * @param c Character to transmit.
 */
void uart_putchar(char);

/**
 * @brief Handler for the external irq triggered by the uart.
 *
 * @note Drains the received chars into the rx buffer, then wakes every
 * process blocked on a uart read.
 */
void uart_irq_handler();

/**
 * @brief Read a character from the rx buffer, without blocking.
 *
 * @param c Where the read character is written.
 * @return 0 if a character was read, -1 if the buffer is empty.
 */
int uart_read(char *c);
