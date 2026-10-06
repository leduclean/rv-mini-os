/**
 * @file
 * @brief Uart driver api interrupt driven on reception.
 */

#pragma once

#define UART_PLIC_ID 10 /**< Uart Interrupt id in the plic */

/**
 * @brief Init the uart to send and receive chars.
 *
 * @note Sets the baud rate, enables the fifo and the RX interrupt.
 */
void uart_init(void);

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
void uart_irq_handler(void);

/**
 * @brief Blocking reader on the rx buffer.
 *
 * @param c Where the read character is written.
 */
void uart_read(char *c);

/**
 * @brief Spawn Uart daemons.
 *
 * @param 0 on SUCCESS, error code < 0 on FAILURE.
 */
int uart_spawn_daemons(void);
