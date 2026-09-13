/**
 * @file
 * @brief Text console drawn on the Bochs display.
 */

#ifndef __CONSOLE_H__
#define __CONSOLE_H__
#include "platform.h"

#define MAX_COLS (DISPLAY_WIDTH / 8) ///< Screen width, in chars.
#define MAX_ROWS (DISPLAY_HEIGHT / 8) ///< Screen height, in chars.

/**
 * @brief Send an output to the uart and to the screen.
 *
 * This is the function called by printf to send its output to the screen.
 * You have to implement it in the kernel and in the user program.
 *
 * @param s Characters to write.
 * @param len Number of characters to write.
 */
void console_putbytes(const char *s, int len);

/**
 * @brief System graphic card configuration.
 *
 * @note Configures the PCIe device then the Bochs display registers.
 *
 * @return 0 on success, -1 if the device was not found or has a wrong type.
 */
int console_init();

/**
 * @brief Display a text on the top right corner of the screen.
 *
 * @note A text longer than MAX_COLS is truncated on its left.
 *
 * @param s Characters to display.
 * @param len Number of characters to display.
 */
void console_display_top_right(const char *s, int len);

#endif
