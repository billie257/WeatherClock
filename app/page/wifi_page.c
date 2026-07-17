#include <stdint.h>
#include <string.h>
#include "st7789.h"
#include "font.h"
#include "image.h"
#include "app.h"

void wifi_page_display(void)
{
		static const char *ssid = WIFI_SSID;
		uint16_t startx = 0;
		int ssid_len = strlen(ssid) * font20_maple_bold.size / 2;
		if (ssid_len <= ST7789_WIDTH)
				startx = (ST7789_WIDTH - ssid_len + 1) / 2;
		
		st7789_fill_color(0, 0, ST7789_WIDTH - 1, ST7789_HEIGHT - 1, BLACK);
		st7789_draw_image(30, 15, &image_wifi);
	  st7789_write_string(88, 158, "WiFi", WHITE, BLACK, &font24_maple_bold);
		st7789_write_string(startx, 233, "vivo S30 Pro mini", mkcolor(241,122,10), BLACK, &font20_maple_bold);
		st7789_write_string(84, 263, "Á¬½ÓÖÐ", WHITE, BLACK, &font24_maple_bold);
}
