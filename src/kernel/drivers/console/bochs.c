#include <stddef.h>
#include <stdint.h>

#include <lib/bit.h>
#include <lib/err.h>
#include <lib/font.h>
#include <lib/mmio.h>
#include <lib/string.h>

#include <drivers/console.h>
#include <drivers/pci.h>
#include <drivers/uart.h>

#include <arch/board.h>
#include <kernel/file.h>
#include <kernel/fs.h>
#include <kernel/mmap.h>
#include <kernel/task.h>
#include <kernel/syscall.h>
#include <kernel/vpages.h>

#include "bochs_regs.h"

#define BOCHS_DISPLAY_BASE_ADDRESS 0x50000000

#define DISPI16(reg_idx) \
	MMIO16(driver.cfg_base + BOCHS_CONFIG_DISPI_ADDRESS + (reg_idx << 1))

#define BG_COLOR 0x000000
#define TEXT_COLOR 0xFFFFFF

struct bochs_driver {
	uintptr_t fb_base; /* framebuffer BAR, programmed by config_pcie() */
	uintptr_t cfg_base; /* config BAR, programmed by config_pcie() */
};

static struct bochs_driver driver;

/**
 * @brief Config BAR and enable pci device.
 *
 * @retval 0 Success.
 * @retval -ENODEV Device missing.
 * @retval -Exxx See pci_get_bar_size(), pci_set_bar().
 */
static int config_pcie(void)
{
	int res;
	int64_t size;
	uint64_t map_addr;
	struct pci_device dev;

	res = pci_find_device(BOCHS_VENDOR_ID, BOCHS_DEV_ID, &dev);
	if (res < 0) {
		return res;
	}

	// Should go check all the cap
	size = pci_get_bar_size(&dev, 0);
	if (size < 0) {
		return size;
	}

	map_addr = ALIGN_UP(BOCHS_DISPLAY_BASE_ADDRESS, size);
	res = pci_set_bar(&dev, 0, map_addr);
	if (res < 0) {
		return res;
	}
	res = iommap_kernel((void *)map_addr, size);
	if (res < 0) {
		return res;
	}
	driver.fb_base = map_addr;

	size = pci_get_bar_size(&dev, 2);
	if (size < 0) {
		return size;
	}
	map_addr = ALIGN_UP(BOCHS_CONFIG_BASE_ADDRESS, size);
	res = pci_set_bar(&dev, 2, map_addr);
	if (res < 0) {
		return res;
	}
	res = iommap_kernel((void *)map_addr, size);
	if (res < 0) {
		return res;
	}
	driver.cfg_base = map_addr;

	pci_enable_device(&dev);

	return 0;
}

/**
 * @brief Config the Bochs display resolution, depth and offsets.
 *
 * @retval 0 Success.
 * @retval -ENODEV Wrong device type id.
 */
static int config_screen(void)
{
	// type id verification (12 MSB comparison)
	if ((DISPI16(VBE_DISPI_INDEX_ID) & 0xFFF0) != VBE_DISPI_ID0) {
		return -ENODEV;
	}
	// Disconnect the screen for config
	DISPI16(VBE_DISPI_INDEX_ENABLE) = 0;

	// Config
	DISPI16(VBE_DISPI_INDEX_XRES) = DISPLAY_WIDTH;
	DISPI16(VBE_DISPI_INDEX_YRES) = DISPLAY_HEIGHT;
	DISPI16(VBE_DISPI_INDEX_BPP) = DISPLAY_BPP;
	DISPI16(VBE_DISPI_INDEX_BANK) = DISPLAY_BANK;
	DISPI16(VBE_DISPI_INDEX_X_OFFSET) = 0;
	DISPI16(VBE_DISPI_INDEX_Y_OFFSET) = 0;

	// ReEnable
	DISPI16(VBE_DISPI_INDEX_ENABLE) = VBE_DISPI_ENABLED |
					  VBE_DISPI_LFB_ENABLED;

	return 0;
};

static ssize_t console_write([[maybe_unused]] struct file *f, const char *ubuf,
			     size_t count)
{
	char kbuf[SYS_BUFFER_SIZE];
	if (count > SYS_BUFFER_SIZE) {
		return -EINVAL;
	}

	int res = vpage_copyin(task_active()->root_ptable, kbuf, ubuf,
			       count);
	if (res < 0) {
		return res;
	}

	console_putbytes(kbuf, count);
	return count;
}

static ssize_t console_read([[maybe_unused]] struct file *f, char *ubuf,
			    size_t count)
{
	if (count > SYS_BUFFER_SIZE) {
		return -EINVAL;
	}
	char kbuf[SYS_BUFFER_SIZE];
	for (size_t i = 0; i < count; i++) {
		uart_read(&kbuf[i]);
	}

	int res = vpage_copyout(task_active()->root_ptable, ubuf, kbuf,
				count);
	if (res < 0) {
		return res;
	}

	return count;
}

int console_init(void)
{
	int res;
	file_map_dev_cb(FILE_DEV_CONSOLE, console_write, console_read);

	res = config_pcie();
	if (res < 0) {
		return res;
	}
	res = config_screen();
	if (res < 0) {
		return res;
	}
	return 0;
};

/**
 * @brief Pixel writing.
 *
 * @param x Column of the pixel, in pixels.
 * @param y Row of the pixel, in pixels.
 * @param color Color to write.
 * @retval 0 Success.
 * @retval -EINVAL Pixel out of range.
 */
static int pixel(uint32_t x, uint32_t y, uint32_t color)
{
	if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT) {
		return -EINVAL;
	}
	uint32_t (*const display_base)[DISPLAY_HEIGHT][DISPLAY_WIDTH] =
		(uint32_t (*)[DISPLAY_HEIGHT][DISPLAY_WIDTH])driver.fb_base;
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
 * @retval 0 Success.
 * @retval -EINVAL Char out of range.
 */
static int write_char(uint32_t row, uint32_t col, char c, uint32_t color,
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
				if (pixel(col + c_off, row + row_off, color) !=
				    0)
					return -EINVAL;
			} else {
				if (pixel(col + c_off, row + row_off,
					   bg_color) != 0)

					return -EINVAL;
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
static void draw_line(uint32_t row, uint32_t col, uint32_t color)
{
	for (int offset = 0; offset < 8; offset++) {
		pixel(col + offset, row, color);
	}
};

static inline void draw_cursor(void)
{
	draw_line(cursor_row * 8 + 7, cursor_col * 8, TEXT_COLOR);
}

static inline void undraw_cursor(void)
{
	draw_line(cursor_row * 8 + 7, cursor_col * 8, BG_COLOR);
}

/** @brief Move the screen display to an upper line. */
static void scroll(void)
{
	// each caractere line has a width of 8 pixel
	uint32_t (*const display_base)[DISPLAY_WIDTH * 8] =
		(uint32_t (*)[DISPLAY_WIDTH * 8]) driver.fb_base;

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
 * @retval 0 Success.
 * @retval -EINVAL @p row is negative.
 */
static int set_cursor(int row, int col)
{
	undraw_cursor();
	while (row >= MAX_ROWS) {
		scroll();
		row--;
	}

	if (row < 0)
		return -EINVAL;

	// Update
	cursor_row = row;
	cursor_col = col;

	// Redraw the cursor
	draw_cursor();
	return 0;
}

/** @brief Move the cursor one char forward, wrapping to the next line. */
static void advance_cursor(void)
{
	if (cursor_col + 1 < MAX_COLS) {
		set_cursor(cursor_row, cursor_col + 1);
	} else {
		set_cursor(cursor_row + 1, 0);
	}
}

/**
 * @brief Draw a char at the cursor position and advance the cursor.
 *
 * @param c Character to draw.
 */
static void put_char(char c)
{
	// write char
	write_char(cursor_row, cursor_col, c, TEXT_COLOR, BG_COLOR);
	advance_cursor();
}

/** @brief Remove all the chars on the screen and reset the cursor. */
static void clear_screen(void)
{
	for (int y = 0; y < DISPLAY_HEIGHT; y++) {
		for (int x = 0; x < DISPLAY_WIDTH; x++) {
			pixel(x, y, BG_COLOR);
		}
	}
	set_cursor(1, 0);
}

// Control char handling helpers

static inline void tab(void)
{
	int distance = (-cursor_col) & 7;
	if (distance + cursor_col >= MAX_COLS) {
		set_cursor(cursor_row + 1, 0);
	} else {
		set_cursor(cursor_row, cursor_col + distance);
	}
}

static inline void backspace(void)
{
	if (cursor_col > 0) {
		set_cursor(cursor_row, cursor_col - 1);
	}
}

static inline void newline(void)
{
	set_cursor(cursor_row + 1, 0);
}
static inline void carriage_return(void)
{
	set_cursor(cursor_row, 0);
}

/**
 * @brief Handle a char type: print it if printable, act on it if control.
 *
 * @param c Character to handle.
 */
static void handle_char(char c)
{
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

void console_putbytes(const char *s, int len)
{
	for (int i = 0; i < len; i++) {
		uart_putchar(s[i]);
		if (driver.fb_base) {
			// Do not use screen until it is initialized.
			// TODO: We could replay this delayed print
			handle_char(s[i]);
		}
	};
};

void console_display_top_right(const char *s, int len)
{
	if (!driver.fb_base) {
		return;
	}

	if (len > MAX_COLS) {
		s += len - MAX_COLS; // remove remaining text
		len = MAX_COLS;
	}

	uint8_t col = MAX_COLS - len;
	for (int i = 0; i < len; i++) {
		write_char(0, col + i, s[i], TEXT_COLOR, BG_COLOR);
	}
}
