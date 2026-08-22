#include <stdint.h>
#include <stdio.h>
#include "FreeRTOS.h"
#include "task.h"
#include "esp_at.h"
#include "page.h"
#include "wifi.h"

#define LOG_TAG "wifi"
#define LOG_LVL ELOG_LVL_INFO
#include "elog.h"

void wifi_init(void)
{
	if (!esp_at_init())
	{
		log_e("[AT] init failed\n");
		goto err;
	}
	
	log_i("[AT] init\n");
	
	if (!esp_at_wifi_init())
	{
		log_e("[WIFI] init failed\n");
		goto err;
	}
	log_i("[WIFI] init\n");
			
	if (!esp_at_sntp_init())
	{
		log_e("[SNTP] init failed\n");
		goto err;
	}
	log_i("[SNTP] init\n");
	
	return;
		
err:
	error_page_display("wireless init failed");
	while (1)
	{
		;
	}
}	

void wifi_wait_connect(void)
{
	log_i("[WIFI] connecting...\n");

	esp_at_connect_wifi(WIFI_SSID, WIFI_PASSWD, NULL);

	for (uint32_t t = 0; t < 10 * 1000; t += 100)
	{
		vTaskDelay(pdMS_TO_TICKS(100));
		esp_wifi_info_t wifi = { 0 };
		if (esp_at_get_wifi_info(&wifi) && wifi.connected)
		{
			log_i("[WiFi] Connected!\n");
			log_i("[WiFi] SSID: %s, BSSID: %s, Channel: %d, RSSI: %d\n",
				  wifi.ssid, wifi.bssid, wifi.channel, wifi.rssi);					
			return;
		}			
	}
	
	log_w("[WiFi] Connection Timeout!\n");
	error_page_display("wireless connect failed");	
	while (1)
	{
			;
	}
}
