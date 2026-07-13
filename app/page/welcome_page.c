#include "st7789.h"
#include "font.h"
#include "image.h"

void welcome_page_display(void)
{
		st7789_fill_color(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1, BLACK);
		st7789_draw_image(30, 13, &image_earth);
	  st7789_write_string(40, 205, "初级嵌入式", mkcolor(237,128,147), BLACK, &font32_maple_bold);
		st7789_write_string(56, 233, "天气时钟", mkcolor(86,165,255), BLACK, &font32_maple_bold);
		st7789_write_string(60, 285, "loading...", WHITE, BLACK, &font24_maple_bold);
}
