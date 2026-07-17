#ifndef __ST7789_H__
#define __ST7789_H__

#include <stdbool.h>
#include <stdint.h>
#include "font.h"
#include "image.h"

#define ST7789_WIDTH    240
#define ST7789_HEIGHT   320

#define RED         0xF800
#define GREEN       0x07E0
#define BLUE        0x001F
#define WHITE       0xFFFF
#define BLACK       0x0000

#define mkcolor(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3))

void st7789_init(void);
void st7789_fill_color(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color);
void st7789_write_string(int16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, const font_t *font);
void st7789_draw_image(uint16_t x, uint16_t y, const image_t *image);

#endif /* __ST7789_H__ */
