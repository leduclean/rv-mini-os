/**
 * @file
 * @brief 8x8 monochrome bitmap font used by the console.
 */

#ifndef _FONT_H_
#define _FONT_H_

/**
 * @brief 8x8 bitmap glyphs for the unicode points U+0000 to U+00FF.
 *
 * Indexed by code point. Each glyph is 8 rows of 8 bits, the least
 * significant bit of a row being its leftmost pixel.
 *
 * @note Public domain font by Daniel Hepper, see font.c for the origin.
 */
extern char font8x8_basic[256][8];

#endif /* _FONT_H_ */
