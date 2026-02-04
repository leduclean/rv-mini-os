#include "mmio.h"
#include "platform.h"

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
  uart_set_baud();
  uart_enable_fifo();
  uart_config_lcr();
  uart_enable_rxirq();
};

void uart_putchar(char c) { MMIO8(UART_BASE + UART_THR) = c; };
char uart_getchar() { return MMIO8(UART_BASE + UART_RBR); };
