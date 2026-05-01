#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include "platform/platform.h"

/**
 * @brief Initialize the text renderer
 */
void text_init();

/**
 * @brief Draw a character at (x, y)
 */
void text_draw_char(int x, int y, char c, Color color);

/**
 * @brief Draw a string at (x, y)
 */
void text_draw_string(int x, int y, const char *str, Color color);

/**
 * @brief Draw a string aligned to the right (x is the right edge)
 */
void text_draw_string_right(int x, int y, const char *str, Color color);

/**
 * @brief Draw a string aligned to the center (x is the center)
 */
void text_draw_string_center(int x, int y, const char *str, Color color);

#endif // TEXT_RENDERER_H
