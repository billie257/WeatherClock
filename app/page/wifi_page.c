#include <stdint.h>
#include <string.h>
#include "ui.h"
#include "app.h"
#include "wifi.h"

void wifi_page_display(void)
{
		static const char *ssid = WIFI_SSID;
		uint16_t startx = 0;
		int ssid_len = strlen(ssid) * font20_maple_bold.size / 2;
		if (ssid_len <= UI_WIDTH)
				startx = (UI_WIDTH - ssid_len + 1) / 2;
		
		ui_fill_color(0, 0, UI_WIDTH - 1, UI_HEIGHT - 1, BLACK);
		ui_draw_image(30, 15, &image_wifi);
	  ui_write_string(88, 158, "WiFi", WHITE, BLACK, &font24_maple_bold);
		ui_write_string(startx, 233, "vivo S30 Pro mini", mkcolor(241,122,10), BLACK, &font20_maple_bold);
		ui_write_string(84, 263, "Á¬½ÓÖÐ", WHITE, BLACK, &font24_maple_bold);
}
