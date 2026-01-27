#include "console.h"
#include "font.h"
#include "platform.h"

// Macro definition for memory mapped container
#define MMIO8(addr) (*(volatile uint8_t *)(addr))
#define MMIO16(addr) (*(volatile uint16_t *)(addr))
#define MMIO32(addr) (*(volatile uint32_t *)(addr))

#define DISPI16(base_addr, reg_idx) MMIO16(base_addr + (reg_idx << 1))
/* Device Command bits
 * bit 0 = memory io
 * bit 1 = memory access
 * bit 2 = screen reach enable
 * */
#define COMMAND_CONFIG 0b111

void init_uart() {
  uint16_t rate = (UART_CLOCK_FREQ / (16 * UART_BAUD_RATE));
  // Load all current value
  uintptr_t lcr_addr = UART_BASE + UART_LCR;
  uint8_t lcr = MMIO8(lcr_addr);
  // Enable rating registers
  lcr |= UART_LCR_DLAB;
  MMIO8(lcr_addr) = lcr;

  // Divisor rate writing
  MMIO8(UART_BASE + UART_DLL) = (uint8_t)(rate);
  MMIO8(UART_BASE + UART_DLH) = (uint8_t)(rate >> 8);

  // fifo writing
  uint8_t fifo = MMIO8(UART_BASE + UART_FCR);
  fifo |= UART_FCR_EWL;
  MMIO8(UART_BASE + UART_FCR) = fifo;

  // 8 bits transmition config
  lcr = MMIO8(lcr_addr);
  lcr |= UART_LCR_8BIT | UART_LCR_PODD; // bits 0, 1,
  lcr &= ~(UART_LCR_DLAB);              // unset bit 7
  MMIO8(lcr_addr) = lcr;
};

void treat_car_uart(char c) { MMIO8(UART_BASE + UART_THR) = c; };

void console_putbytes(const char *s, int len) {
  // uart init
  init_uart();
  for (int i = 0; i < len; i++) {
    treat_car_uart(s[i]);
  };
};

// Make PCI ECAM address
static inline uintptr_t make_device_addr(uint32_t bus, uint32_t dev,
                                         uint32_t func, uint32_t offset) {
  return PCI_ECAM_BASE_ADDRESS | (bus << PCI_BUS_SHIFT) |
         (dev << PCI_DEVICE_SHIFT) | (func << PCI_FUNC_SHIFT) | offset;
}

int config_pcie() {
  int found = -1;
  uintptr_t device_addr;
  for (uint32_t dev = 0; dev <= 31; dev++) {
    device_addr = make_device_addr(0, dev, 0, 0);
    if (MMIO32(device_addr) == DISPLAY_PCI_ID) {
      found = 0;
      break;
    }
  };

  if (found != 0) {
    return -1; // error
  }
  // Command setting for io, memory acces and screen reaching
  MMIO32(device_addr + PCI_DEV_COMMAND) |= COMMAND_CONFIG;

  // Base addr config for display and config
  MMIO32(device_addr + PCI_DEV_DISPLAY_ADDR) = BOCHS_DISPLAY_BASE_ADDRESS;
  MMIO32(device_addr + PCI_DEV_CONFIG_ADDR) = BOCHS_CONFIG_BASE_ADDRESS;

  return 0;
};

int config_screen() {
  uintptr_t dispi_base = BOCHS_CONFIG_BASE_ADDRESS + BOCHS_CONFIG_DISPI_ADDRESS;
  // type id verification (12 MSB comparison)
  if ((MMIO16(dispi_base) & 0xFFF0) != VBE_DISPI_ID0) {
    return -1; // Error wrong screen device type
  }
  // Disconnect the screen for config
  DISPI16(dispi_base, VBE_DISPI_INDEX_ENABLE) = 0;

  // Config
  DISPI16(dispi_base, VBE_DISPI_INDEX_XRES) = DISPLAY_WIDTH;
  DISPI16(dispi_base, VBE_DISPI_INDEX_YRES) = DISPLAY_HEIGHT;
  DISPI16(dispi_base, VBE_DISPI_INDEX_BPP) = DISPLAY_BPP;
  DISPI16(dispi_base, VBE_DISPI_INDEX_BANK) = DISPLAY_BANK;
  DISPI16(dispi_base, VBE_DISPI_INDEX_X_OFFSET) = 0;
  DISPI16(dispi_base, VBE_DISPI_INDEX_Y_OFFSET) = 0;

  // ReEnable
  DISPI16(dispi_base, VBE_DISPI_INDEX_ENABLE) =
      VBE_DISPI_ENABLED | VBE_DISPI_LFB_ENABLED;

  return 0;
};

/*
 * System graphic card configuration
 * */
int init_screen() {
  if (config_pcie() != 0)
    return -1;
  if (config_screen() != 0)
    return -1;
  return 0;
};

/* Pixel writing */
int pixel(uint32_t x, uint32_t y, uint32_t color) {
  if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
    return -1; // error out of range
  }
  static uint32_t (*const display_base)[DISPLAY_HEIGHT][DISPLAY_WIDTH] =
      (uint32_t (*const)[DISPLAY_HEIGHT][DISPLAY_WIDTH])
          BOCHS_DISPLAY_BASE_ADDRESS;
  // usage
  (*display_base)[y][x] = color;

  return 0; // success
};

/* Char writing */
int write_car(uint32_t row, uint32_t col, char c, uint32_t color,
              uint32_t bg_color) {
  char *tab = font8x8_basic[(uint8_t)c];
  for (int row_off = 0; row_off < 8; row_off++) {
    for (int c_off = 0; c_off < 8; c_off++) {
      if ((uint8_t)tab[row_off] & (1 << c_off)) {
        if (pixel(col + c_off, row + row_off, color) != 0)
          return -1;
      } else {
        if (pixel(col + c_off, row + row_off, bg_color) != 0)
          return -1;
      }
    }
  }
  return 0;
};

/* Cursor position handling */
void set_cursor(uint32_t lig, uint32_t col) {}
