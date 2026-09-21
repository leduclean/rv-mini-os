#pragma once

#define UART_CLOCK_FREQ 1843200
#define UART_BAUD_RATE 115200

#define UART_IER_RDI 0x01 /* Enable receiver data interrupt */
#define UART_IIR_NO_INT 0x01 /* No interrupts pending */
#define UART_IIR_ID 0x06 /* Mask for the interrupt ID */

/* ns16550 registers */
#define UART_RBR 0x00 /* Receive Buffer Register */
#define UART_THR 0x00 /* Transmit Hold Register */
#define UART_IER 0x01 /* Interrupt Enable Register */
#define UART_DLL 0x00 /* Divisor LSB (LCR_DLAB) */
#define UART_DLH 0x01 /* Divisor MSB (LCR_DLAB) */
#define UART_FCR 0x02 /* FIFO Control Register */
#define UART_LCR 0x03 /* Line Control Register */
#define UART_MCR 0x04 /* Modem Control Register */
#define UART_LSR 0x05 /* Line Status Register */
#define UART_MSR 0x06 /* Modem Status Register */
#define UART_SCR 0x07 /* Scratch Register */

/* Interrupt Enable Register bitmap */
#define UART_IER_RXEN (1 << 0) /* Received Data Available interrupt */
#define UART_IER_TXEN (1 << 1) /* Tramistter Holding Register empty */

/* Line Status Register  bitmap */
#define UART_LSR_DA (1 << 0) /* Data Available */
#define UART_LSR_OE (1 << 1) /* Overrun Error */
#define UART_LSR_PE (1 << 2) /* Parity Error */
#define UART_LSR_FE (1 << 3) /* Framing Error */
#define UART_LSR_BI (1 << 4) /* Break indicator */
#define UART_LSR_RE (1 << 5) /* THR is empty */
#define UART_LSR_RI (1 << 6) /* THR is empty and line is idle */
#define UART_LSR_EF (1 << 7) /* Erroneous data in FIFO */

/* Fifo Control Register bitmap */
#define UART_FCR_EWL (1 << 0) /* Waiting list enable Bit */

/* Line Control Register bitmap */
#define UART_LCR_8BIT 0b11 /* 8-bit */
#define UART_LCR_PEN (1 << 3) /* Parity Enable */
#define UART_LCR_DLAB (1 << 7) /* Divisor Latch Bit */
