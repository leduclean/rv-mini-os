#include <stdint.h>

#include <lib/font.h>
#include <lib/string.h>

#include <asm/console.h>
#include <asm/mmio.h>
#include <asm/uart.h>

#define DISPI16(base_addr, reg_idx) MMIO16(base_addr + (reg_idx << 1))
/** @brief Device command bits: memory io, memory access, screen reach. */
#define COMMAND_CONFIG 0b111

#define BG_COLOR 0x000000
#define TEXT_COLOR 0xFFFFFF

/**
 * @brief Make a PCI ECAM address.
 *
 * @param bus Bus number.
 * @param dev Device number.
 * @param func Function number.
 * @param offset Offset of the register in the config space.
 * @return The assembled ECAM address.
 */
static inline uintptr_t _make_device_addr(uint32_t bus, uint32_t dev,
					  uint32_t func, uint32_t offset)
{
	return PCI_ECAM_BASE_ADDRESS | (bus << PCI_BUS_SHIFT) |
	       (dev << PCI_DEVICE_SHIFT) | (func << PCI_FUNC_SHIFT) | offset;
}

/**
 * @brief Find the display device on the PCIe bus and map its base addresses.
 *
 * @return 0 on success, -1 if no display device was found.
 */
static int _config_pcie()
{
	int found = -1;
	uintptr_t device_addr;
	for (uint32_t dev = 0; dev <= 31; dev++) {
		device_addr = _make_device_addr(0, dev, 0, 0);
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

/**
 * @brief Config the Bochs display resolution, depth and offsets.
 *
 * @return 0 on success, -1 if the device has a wrong type id.
 */
static int _config_screen()
{
	uintptr_t dispi_base = BOCHS_CONFIG_BASE_ADDRESS +
			       BOCHS_CONFIG_DISPI_ADDRESS;
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
	DISPI16(dispi_base, VBE_DISPI_INDEX_ENABLE) = VBE_DISPI_ENABLED |
						      VBE_DISPI_LFB_ENABLED;

	return 0;
};

int console_init()
{
	if (_config_pcie() != 0)
		return -1;
	if (_config_screen() != 0)
		return -1;
	return 0;
};

/**
 * @brief Pixel writing.
 *
 * @param x Column of the pixel, in pixels.
 * @param y Row of the pixel, in pixels.
 * @param color Color to write.
 * @return 0 on success, -1 if the pixel is out of range.
 */
static int _pixel(uint32_t x, uint32_t y, uint32_t color)
{
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

/**
 * @brief Char writing.
 *
 * @param row Row of the char, in chars.
 * @param col Column of the char, in chars.
 * @param c Character to draw.
 * @param color Color of the glyph.
 * @param bg_color Color of the background.
 * @return 0 on success, -1 if the char is out of range.
 */
static int _write_char(uint32_t row, uint32_t col, char c, uint32_t color,
		       uint32_t bg_color)
{
	// Pixel conversion
	row *= 8;
	col *= 8;

	// Load font
	char *tab = font8x8_basic[(uint8_t)c];

	// Color the pixels of the 8x8 char
	for (int row_off = 0; row_off < 8; row_off++) {
		for (int c_off = 0; c_off < 8; c_off++) {
			if ((uint8_t)tab[row_off] & (1 << c_off)) {
				if (_pixel(col + c_off, row + row_off, color) !=
				    0)
					return -1;
			} else {
				if (_pixel(col + c_off, row + row_off,
					   bg_color) != 0)
					return -1;
			}
		}
	}
	return 0;
};

// Cursor position init
static uint8_t cursor_row = 1;
static uint8_t cursor_col = 0;

/**
 * @brief Cursor position handling, drawing an 8 pixel horizontal line.
 *
 * @param row Row of the line, in pixels.
 * @param col Column the line starts at, in pixels.
 * @param color Color of the line.
 */
static void _draw_line(uint32_t row, uint32_t col, uint32_t color)
{
	for (int offset = 0; offset < 8; offset++) {
		_pixel(col + offset, row, color);
	}
};

static inline void _draw_cursor()
{
	_draw_line(cursor_row * 8 + 7, cursor_col * 8, TEXT_COLOR);
}

static inline void _undraw_cursor()
{
	_draw_line(cursor_row * 8 + 7, cursor_col * 8, BG_COLOR);
}

/** @brief Move the screen display to an upper line. */
static void _scroll()
{
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

/**
 * @brief Move the cursor, scrolling the screen if it goes past the last row.
 *
 * @param row New row of the cursor, in chars.
 * @param col New column of the cursor, in chars.
 * @return 0 on success, -1 if @p row is negative.
 */
static int _set_cursor(int row, int col)
{
	_undraw_cursor();
	while (row >= MAX_ROWS) {
		_scroll();
		row--;
	}

	if (row < 0)
		return -1; // Not Supported

	// Update
	cursor_row = row;
	cursor_col = col;

	// Redraw the cursor
	_draw_cursor();
	return 0;
}

/** @brief Move the cursor one char forward, wrapping to the next line. */
static void _advance_cursor()
{
	if (cursor_col + 1 < MAX_COLS) {
		_set_cursor(cursor_row, cursor_col + 1);
	} else {
		_set_cursor(cursor_row + 1, 0);
	}
}

/**
 * @brief Draw a char at the cursor position and advance the cursor.
 *
 * @param c Character to draw.
 */
static void _put_char(char c)
{
	// write char
	_write_char(cursor_row, cursor_col, c, TEXT_COLOR, BG_COLOR);
	_advance_cursor();
}

/** @brief Remove all the chars on the screen and reset the cursor. */
static void _clear_screen()
{
	for (int y = 0; y < DISPLAY_HEIGHT; y++) {
		for (int x = 0; x < DISPLAY_WIDTH; x++) {
			_pixel(x, y, BG_COLOR);
		}
	}
	_set_cursor(1, 0);
}

// Control char handling helpers

static inline void _tab()
{
	int distance = (-cursor_col) & 7;
	if (distance + cursor_col >= MAX_COLS) {
		_set_cursor(cursor_row + 1, 0);
	} else {
		_set_cursor(cursor_row, cursor_col + distance);
	}
}

static inline void _backspace()
{
	if (cursor_col > 0) {
		_set_cursor(cursor_row, cursor_col - 1);
	}
}

static inline void _newline()
{
	_set_cursor(cursor_row + 1, 0);
}
static inline void _carriage_return()
{
	_set_cursor(cursor_row, 0);
}

/**
 * @brief Handle a char type: print it if printable, act on it if control.
 *
 * @param c Character to handle.
 */
static void _handle_char(char c)
{
	// We only treat those char
	if (c >= 32 && c < 127) {
		_put_char(c);
	} else {
		// Control char handling
		switch (c) {
		case '\b':
			_backspace();
			break;
		case '\t':
			_tab();
			break;
		case '\n':
			_newline();
			break;
		case '\f':
			_clear_screen();
			break;
		case '\r':
			_carriage_return();
			break;
		}
	}
};

void console_putbytes(const char *s, int len)
{
	for (int i = 0; i < len; i++) {
		uart_putchar(s[i]);
		_handle_char(s[i]);
	};
};

void console_display_top_right(const char *s, int len)
{
	if (len > MAX_COLS) {
		s += len - MAX_COLS; // remove remaining text
		len = MAX_COLS;
	}

	uint8_t col = MAX_COLS - len;
	for (int i = 0; i < len; i++) {
		uart_putchar(s[i]);
		_write_char(0, col + i, s[i], TEXT_COLOR, BG_COLOR);
	}
}
