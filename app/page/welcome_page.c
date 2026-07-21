#include "ui.h"

void welcome_page_display(void)
{
		ui_fill_color(0, 0, UI_WIDTH - 1, UI_HEIGHT - 1, BLACK);
		ui_draw_image(30, 13, &image_earth);
	  ui_write_string(40, 205, "初级嵌入式", mkcolor(237,128,147), BLACK, &font32_maple_bold);
		ui_write_string(56, 233, "天气时钟", mkcolor(86,165,255), BLACK, &font32_maple_bold);
		ui_write_string(60, 285, "loading...", WHITE, BLACK, &font24_maple_bold);
}
