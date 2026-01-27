#include "console.h"
#include "platform.h"

// Macro definition for memory mapped container 
#define MMIO8(addr) (*(volatile uint8_t *)(addr))

void init_uart() {
    uint16_t rate = (UART_CLOCK_FREQ / (16 * UART_BAUD_RATE));
    // Load all current value
    uintptr_t lcr_addr = UART_BASE + UART_LCR; 
    uint8_t lcr = MMIO8(lcr_addr);
    // Enable rating registers  
    lcr |= 0x80; 
    MMIO8(lcr_addr) = lcr;

    // Divisor rate writing 
    MMIO8(UART_BASE + UART_DLL) = (uint8_t)(rate);
    MMIO8(UART_BASE + UART_DLH) = (uint8_t)(rate >> 8);

    // fifo writing 
    uint8_t fifo = MMIO8(UART_BASE + UART_FCR);
    fifo |= 0x01; //bit 0 
    MMIO8(UART_BASE +UART_FCR) = fifo;
 
    // 8 bits transmition config 
    lcr = MMIO8(lcr_addr);
    lcr |= 0x07; // bits 0, 1, 
    lcr &= ~(0x80); // unset bit 7
    MMIO8(lcr_addr) = lcr;
};

void treat_car_uart(char c) {
    MMIO8(UART_BASE + UART_THR) = c;
};

void console_putbytes(const char *s, int len) {
    // uart init 
    init_uart();
    for (int i = 0; i < len; i++) {
        treat_car_uart(s[i]);
    };
};
