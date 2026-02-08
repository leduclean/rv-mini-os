#include "drivers/uart.h"
#include "arch/riscv/mmio.h"
#include "arch/riscv/platform.h"
#include "kernel/process/process.h"
#include "kernel/sched/scheduler.h"
#include "lib/stdint.h"
#include "lib/string.h"

#define UART_RX_BUFFER_SIZE 128

/** Sleeping queue relative to uart IO.
 * All of it process should be waken on irq.
 **/
static wait_queue_t uart_wait_queue;

static inline void uart_init_queue() {
  uart_wait_queue.head = NULL;
  uart_wait_queue.tail = NULL;
}

wait_queue_t *uart_get_wait_queue() { return &uart_wait_queue; }

/** We define a ring buffer to read the inner char **/
typedef struct {
  char buf[UART_RX_BUFFER_SIZE];
  volatile uint32_t head;
  volatile uint32_t tail;
} uart_ring_buffer_t;

static uart_ring_buffer_t rx_buffer;

static inline int buffer_empty(uart_ring_buffer_t *b) {
  return b->head == b->tail;
}

static inline int buffer_full(uart_ring_buffer_t *b) {
  return ((b->head + 1) % UART_RX_BUFFER_SIZE) == b->tail;
}

static inline void buffer_put(uart_ring_buffer_t *b, char c) {
  if (!buffer_full(b)) {
    b->buf[b->head] = c;
    b->head = (b->head + 1) % UART_RX_BUFFER_SIZE;
  }
}

static inline char buffer_get(uart_ring_buffer_t *b) {
  if (!buffer_empty(b)) {
    char c = b->buf[b->tail];
    b->tail = (b->tail + 1) % UART_RX_BUFFER_SIZE;
    return c;
  }
  return 0;
}

static inline void uart_enable_fifo() {
  // Enable fifo waiting list
  MMIO8(UART_BASE + UART_FCR) = UART_FCR_EWL;
}

static inline void uart_config_lcr() {
  // 8 bits transmition/reception config
  MMIO8(UART_BASE + UART_LCR) |= UART_LCR_8BIT | UART_LCR_PODD; // bits 0, 1, 3
}

static void uart_set_baud() {
  uint16_t rate = (UART_CLOCK_FREQ / (16 * UART_BAUD_RATE));
  uintptr_t lcr_addr = UART_BASE + UART_LCR;

  // Enable divisor latches
  MMIO8(lcr_addr) |= UART_LCR_DLAB;

  // Divisor rate writing
  MMIO8(UART_BASE + UART_DLL) = (uint8_t)(rate);
  MMIO8(UART_BASE + UART_DLH) = (uint8_t)(rate >> 8);

  // Disable divisor latches to acces THR, RBR, IER
  MMIO8(lcr_addr) &= ~UART_LCR_DLAB;
}

/** Enable the RX interupt **/
static inline void uart_enable_rxirq() {
  MMIO8(UART_BASE + UART_IER) |= UART_RXEN;
}

/** Init the uart to receive char and get char **/
void uart_init() {
  uart_init_queue();
  uart_set_baud();
  uart_enable_fifo();
  uart_config_lcr();
  uart_enable_rxirq();
};

/* Write a character in the THR FIFO buffer (a transmitted char) */
void uart_putchar(char c) { MMIO8(UART_BASE + UART_THR) = c; };

/* Get a character in the RBR FIFO buffer (a received char)*/
static char uart_getchar() { return MMIO8(UART_BASE + UART_RBR); };

/* Signals that data is available in rx */
static inline uint8_t uart_rx_data_ready() {
  return MMIO8(UART_BASE + UART_LSR) & 1;
}

/* Fill the rx buffer while rx contain data */
static void uart_fill_rx_buff() {
  while (uart_rx_data_ready()) {
    buffer_put(&rx_buffer, uart_getchar());
  }
}

/* Read the data in the RX buffer if available*/
int uart_read(char *c) {
  if (!buffer_empty(&rx_buffer)) {
    *c = buffer_get(&rx_buffer);
    return 0;
  }
  return -1;
}

/* Handler for the external interupt trigerred by the uart */
void uart_irq_handler() {
  uart_fill_rx_buff();
  scheduler_wake_waiting_queue(&uart_wait_queue);
}
