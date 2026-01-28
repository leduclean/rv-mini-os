#include "console.h"
#include "font.h"
#include "include/mmio.h"
#include "platform.h"
#include <stdint.h>
#include <string.h>

#define DISPI16(base_addr, reg_idx) MMIO16(base_addr + (reg_idx << 1))
/* Device Command bits
 * bit 0 = memory io
 * bit 1 = memory access
 * bit 2 = screen reach enable
 * */
#define COMMAND_CONFIG 0b111

#define BG_COLOR 0x000000
#define TEXT_COLOR 0xFFFFFF

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
int init_ecran() {
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
int write_char(uint32_t row, uint32_t col, char c, uint32_t color,
               uint32_t bg_color) {
  // Pixel conversion
  row *= 8;
  col *= 8;

  // Load font
  char *tab = font8x8_basic[(uint8_t)c];

  // Color the pixels of the 8x8 char
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

// Cursor position init
uint8_t cursor_row = 1;
uint8_t cursor_col = 0;

/* Cursor position handling */
void draw_line(uint32_t row, uint32_t col, uint32_t color) {
  for (int offset = 0; offset < 8; offset++) {
    pixel(col + offset, row, color);
  }
};

static inline void draw_cursor() {
  draw_line(cursor_row * 8 + 7, cursor_col * 8, TEXT_COLOR);
}

static inline void undraw_cursor() {
  draw_line(cursor_row * 8 + 7, cursor_col * 8, BG_COLOR);
}

#define MAX_COLS (DISPLAY_WIDTH / 8)
#define MAX_ROWS (DISPLAY_HEIGHT / 8)

/* Move screen display to an upper line */
void defilement() {
  // each caractere line has a width of 8 pixel
  static uint32_t (*const display_base)[DISPLAY_WIDTH * 8] =
      (uint32_t (*)[DISPLAY_WIDTH * 8]) BOCHS_DISPLAY_BASE_ADDRESS;

  // Move all the line to up
  memmove(display_base, display_base + 1,
          (DISPLAY_HEIGHT - 1) * DISPLAY_WIDTH * sizeof(uint32_t));

  // Remove last line
  memset(display_base[DISPLAY_HEIGHT - 1], BG_COLOR,
         1 * DISPLAY_WIDTH * sizeof(uint32_t));
}

int set_cursor(int row, int col) {
  undraw_cursor();
  while (row >= MAX_ROWS) {
    defilement();
    row--;
  }

  if (row < 0)
    return -1; // Not Supported

  // Update
  cursor_row = row;
  cursor_col = col;

  // Redraw the cursor
  draw_cursor();
  return 0;
}

void advance_cursor() {
  if (cursor_col + 1 < MAX_COLS) {
    set_cursor(cursor_row, cursor_col + 1);
  } else {
    set_cursor(cursor_row + 1, 0);
  }
}

void put_char(char c) {
  // write char
  write_char(cursor_row, cursor_col, c, TEXT_COLOR, BG_COLOR);
  advance_cursor();
}

/* Remove all the char on the screen */
void clear_screen() {
  for (int y = 0; y < DISPLAY_HEIGHT; y++) {
    for (int x = 0; x < DISPLAY_WIDTH; x++) {
      pixel(x, y, BG_COLOR);
    }
  }
  set_cursor(1, 0);
}

/*/
 * Control char handling helpers
 */

static inline void tab() {
  int distance = (-cursor_col) & 7;
  if (distance + cursor_col >= MAX_COLS) {
    set_cursor(cursor_row + 1, 0);
  } else {
    set_cursor(cursor_row, cursor_col + distance);
  }
}

static inline void backspace() {
  if (cursor_col > 0) {
    set_cursor(cursor_row, cursor_col - 1);
  }
}

static inline void newline() { set_cursor(cursor_row + 1, 0); }
static inline void carriage_return() { set_cursor(cursor_row, 0); }

/* Handle char type -> print if char or do action if control */
void handle_char(char c) {
  // We only treat those char
  if (c >= 32 && c < 127) {
    put_char(c);
  } else {
    // Control char handling
    switch (c) {
    case '\b':
      backspace();
      break;
    case '\t':
      tab();
      break;
    case '\n':
      newline();
      break;
    case '\f':
      clear_screen();
      break;
    case '\r':
      carriage_return();
      break;
    }
  }
};

/* Main fonction used to pipe char to UART and to screen dipslay */
void console_putbytes(const char *s, int len) {
  for (int i = 0; i < len; i++) {
    treat_car_uart(s[i]);
    handle_char(s[i]);
  };
};
