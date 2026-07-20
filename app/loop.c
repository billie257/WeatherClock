#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"
#include "cpu_tick.h"
#include "rtc.h"
#include "esp_at.h"
#include "app.h"
#include "page.h"
#include "aht20.h"
#include "weather.h"

#define MILLISECONDS(x)   (x)
#define SECONDS(x)        MILLISECONDS((x) * 1000)
#define MINUTES(x)        SECONDS((x) * 60)
#define HOURS(x)          MINUTES((x) * 60)
#define DAYS(x)           HOURS((x) * 24)

#define TIME_SYNC_INTERVAL         HOURS(1)    
#define WIFI_UPDATE_INTERVAL       SECONDS(5)
#define TIME_UPDATE_INTERVAL       SECONDS(1)
#define INNER_UPDATE_INTERVAL      SECONDS(3)
#define OUTDOOR_UPDATE_INTERVAL    MINUTES(1)

#define LOOP_EVT_TIME_SYNC   	    (1 << 0)
#define LOOP_EVT_WIFI_UPDATE   	  (1 << 1)
#define LOOP_EVT_TIME_UPDATE   	  (1 << 2)
#define LOOP_EVT_INNER_UPDATE   	(1 << 3)
#define LOOP_EVT_OUTDOOR_UPDATE   (1 << 4)
#define LOOP_EVT_ALL              (LOOP_EVT_TIME_SYNC   	      |\
																	 LOOP_EVT_WIFI_UPDATE         |\
																	 LOOP_EVT_TIME_UPDATE         |\
																	 LOOP_EVT_INNER_UPDATE        |\
																	 LOOP_EVT_OUTDOOR_UPDATE)

static TaskHandle_t loop_task;
static TimerHandle_t time_sync_timer;
static TimerHandle_t wifi_update_timer;
static TimerHandle_t time_update_timer;
static TimerHandle_t inner_update_timer;
static TimerHandle_t outdoor_update_timer;

static void time_sync(void)
{		
		uint32_t restart_sync_delay = TIME_SYNC_INTERVAL;
		rtc_date_time_t rtc_date = { 0 };
	
		esp_date_time_t esp_date = { 0 };
		if (!esp_at_sntp_get_time(&esp_date))
		{
				printf("[SNTP] get time failed\n");
				restart_sync_delay = SECONDS(1);
				goto err;
		}
		
		if (esp_date.year < 2020)
		{
				printf("[SNTP] invalid date formate\n");
				restart_sync_delay = SECONDS(1);
				goto err;
		}
		
		printf("[SNTP] sync time: %04u-%02u-%02u %02u:%02u:%02u (%d)\n", 
					esp_date.year, esp_date.month, esp_date.day,
					esp_date.hour, esp_date.minute, esp_date.second, esp_date.weekday);
		
		rtc_date.year = esp_date.year;
		rtc_date.month = esp_date.month;
		rtc_date.day = esp_date.day;
		rtc_date.hour = esp_date.hour;
		rtc_date.minute = esp_date.minute;
		rtc_date.second = esp_date.second;
		rtc_date.weekday = esp_date.weekday;		
		rtc_set_time(&rtc_date);
		
err:
		xTimerChangePeriod(time_sync_timer, pdMS_TO_TICKS(restart_sync_delay), 0);
		xTaskNotify(loop_task, LOOP_EVT_TIME_UPDATE, eSetBits);
}

static void wifi_update(void)
{
		static esp_wifi_info_t last_info = { 0 };
	
		esp_wifi_info_t info = { 0 };
		if (!esp_at_get_wifi_info(&info))
		{
				printf("[AT] Wifi info get failed\n");
				return;
		}
		
		if (memcmp(&info, &last_info, sizeof(esp_wifi_info_t)) == 0)
			return;
		if (last_info.connected == info.connected)
			return;
		
		if (info.connected)
		{
				printf("[WIFI] connected to %s\n", info.ssid);
				printf("[WIFI] SSID: %s, BSSID: %s, Channel: %d, RSSI: %d\n",
								info.ssid, info.bssid, info.channel, info.rssi);
			
				main_page_redraw_wifi_ssid(info.ssid);
		}
		else
		{
			 printf("[WIFI] disconnected from %s\n", last_info.ssid);
			 main_page_redraw_wifi_ssid("wifi lost");
		}
		
		memcpy(&last_info, &info, sizeof(esp_wifi_info_t));
		
		xTimerChangePeriod(wifi_update_timer, pdMS_TO_TICKS(WIFI_UPDATE_INTERVAL), 0);		
}

static void time_update(void)
{
		static rtc_date_time_t last_date = { 0 };
		
		xTimerChangePeriod(time_update_timer, pdMS_TO_TICKS(TIME_UPDATE_INTERVAL), 0);		
		
		rtc_date_time_t date;
		rtc_get_time(&date);
		
		if (memcmp(&last_date, &date, sizeof(rtc_date_time_t)) == 0)
				return;
		
		memcpy(&last_date, &date, sizeof(rtc_date_time_t));
		main_page_redraw_time(&date);
		main_page_redraw_date(&date);		
}

static void inner_update(void)
{
		static float last_temperature, last_humidity;
		
		xTimerChangePeriod(inner_update_timer, pdMS_TO_TICKS(INNER_UPDATE_INTERVAL), 0);		
		
		if (!aht20_start_measurement())
		{
				printf("[AHT20] start measurement failed\n");
				return;
		}
		
		if (!aht20_wait_for_measurement())
		{
				printf("[AHT20] wait for measurement failed\n");
				return;
		}
		
		float temperature = 0.0f, humidity = 0.0f;
		
		if (!aht20_read_measurement(&temperature, &humidity))
		{
				printf("[AHT20] read measurement failed\n");
				return;
		}
		
		if (temperature == last_temperature && humidity == last_humidity)
			return;
		
		last_temperature = temperature;
		last_humidity = humidity;
		
		printf("[AHT20] Temperature: %.1f, Humidiy: %.1f\r\n", temperature, humidity);
		main_page_redraw_inner_temperature(temperature);
		main_page_redraw_inner_humidity(humidity);
}

static void outdoor_update(void)
{
		static weather_info_t last_weather = { 0 };
		
		xTimerChangePeriod(outdoor_update_timer, pdMS_TO_TICKS(OUTDOOR_UPDATE_INTERVAL), 0);		
		weather_info_t weather = { 0 };
		const char *weather_url = WEATHER_URL;
		const char *weather_http_response = esp_at_http_get(weather_url);
		if (weather_http_response == NULL)
		{
				printf("[WEATHER] http error!\n");
				return;
		}
		
		if (!parse_seniverse_response(weather_http_response, &weather))
		{
				printf("[WEATHER] parse failed!\n");
				return;
		}
		
		if (memcmp(&last_weather, &weather, sizeof(weather_info_t)) == 0)
				return;
		
		memcpy(&last_weather, &weather, sizeof(weather_info_t));
		printf("[WEATHER] %s, %s, %.1f\n", weather.city, weather.weather, weather.temperature);
		main_page_redraw_outdoor_temperature(weather.temperature);
		main_page_redraw_outdoor_weather_icon(weather.weather_code);
}

static void loop_func(void *param)
{
		uint32_t event;
	
		while (1)
		{
				event = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
			
				if (event & LOOP_EVT_TIME_SYNC)
						time_sync();
				if (event & LOOP_EVT_WIFI_UPDATE)
						wifi_update();
				if (event & LOOP_EVT_TIME_UPDATE)
						time_update();
				if (event & LOOP_EVT_INNER_UPDATE)
						inner_update();	
				if (event & LOOP_EVT_OUTDOOR_UPDATE)
						outdoor_update();			
	}
}


static void loop_timer_cb(TimerHandle_t timer)
{
		uint32_t event = (uint32_t)pvTimerGetTimerID(timer);
		xTaskNotify(loop_task, event, eSetBits);
}

void main_loop_init(void)
{
		time_sync_timer = xTimerCreate("time sync", 1, pdFALSE, (void *)LOOP_EVT_TIME_SYNC, loop_timer_cb);
		wifi_update_timer = xTimerCreate("wifi update", pdMS_TO_TICKS(WIFI_UPDATE_INTERVAL), pdTRUE, (void *)LOOP_EVT_WIFI_UPDATE, loop_timer_cb);
		time_update_timer = xTimerCreate("time update", pdMS_TO_TICKS(TIME_UPDATE_INTERVAL), pdTRUE, (void *)LOOP_EVT_TIME_UPDATE, loop_timer_cb);
		inner_update_timer = xTimerCreate("inner update", pdMS_TO_TICKS(INNER_UPDATE_INTERVAL), pdTRUE, (void *)LOOP_EVT_INNER_UPDATE, loop_timer_cb);
		outdoor_update_timer = xTimerCreate("outdoor update", pdMS_TO_TICKS(OUTDOOR_UPDATE_INTERVAL), pdTRUE, (void *)LOOP_EVT_OUTDOOR_UPDATE, loop_timer_cb);	

	  xTaskCreate(loop_func, "loop", 1024, NULL, 5, &loop_task);
		xTaskNotify(loop_task, LOOP_EVT_ALL, eSetBits);

		xTimerStart(wifi_update_timer, 0);
	  xTimerStart(time_update_timer, 0);
	  xTimerStart(inner_update_timer, 0);
	  xTimerStart(outdoor_update_timer, 0);
}
