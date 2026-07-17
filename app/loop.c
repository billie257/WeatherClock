#include <stdint.h>
#include <stdio.h>
#include <string.h>
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

#define TIME_SYNC_INTERVAL         DAYS(1)    
#define WIFI_UPDATE_INTERVAL       SECONDS(5)
#define TIME_UPDATE_INTERVAL       SECONDS(1)
#define INNER_UPDATE_INTERVAL      SECONDS(3)
#define OUTDOOR_UPDATE_INTERVAL    MINUTES(1)

static uint32_t time_sync_delay;
static uint32_t wifi_update_delay;
static uint32_t time_update_delay;
static uint32_t inner_update_delay;
static uint32_t outdoor_update_delay;

static void cpu_periodic_callback(void)
{
		if (time_sync_delay > 0)
				time_sync_delay--;
		if (wifi_update_delay > 0)
				wifi_update_delay--;
		if (time_update_delay > 0)
				time_update_delay--;
		if (inner_update_delay > 0)
				inner_update_delay--;
		if (outdoor_update_delay > 0)
				outdoor_update_delay--;
}

void main_loop_init(void)
{
		cpu_register_periodic_callback(cpu_periodic_callback);
}

static void time_sync(void)
{
		if (time_sync_delay > 0)
			return;
		
		time_sync_delay = TIME_SYNC_INTERVAL;	
		
		esp_date_time_t esp_date = { 0 };
		if (!esp_at_sntp_get_time(&esp_date))
		{
				printf("[SNTP] get time failed\n");
				time_sync_delay = SECONDS(5);
				return;
		}
		
		if (esp_date.year < 2020)
		{
				printf("[SNTP] invalid date formate\n");
				time_sync_delay = SECONDS(5);
				return;
		}
		
		printf("[SNTP] sync time: %04u-%02u-%02u %02u:%02u:%02u (%d)\n", 
					esp_date.year, esp_date.month, esp_date.day,
					esp_date.hour, esp_date.minute, esp_date.second, esp_date.weekday);
		
		rtc_date_time_t rtc_date = { 0 };
		rtc_date.year = esp_date.year;
		rtc_date.month = esp_date.month;
		rtc_date.day = esp_date.day;
		rtc_date.hour = esp_date.hour;
		rtc_date.minute = esp_date.minute;
		rtc_date.second = esp_date.second;
		rtc_date.weekday = esp_date.weekday;		
		rtc_set_time(&rtc_date);
		
		time_update_delay = 10;
}

static void wifi_update(void)
{
		static esp_wifi_info_t last_info = { 0 };
	
		if (wifi_update_delay > 0)
			return;
		
		wifi_update_delay = WIFI_UPDATE_INTERVAL;	
		
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
}

static void time_update(void)
{
		static rtc_date_time_t last_date = { 0 };
		
		if (time_update_delay > 0)
				return;
		
		time_update_delay = TIME_UPDATE_INTERVAL;
		
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
	
		if (inner_update_delay > 0)
				return;
		
		inner_update_delay = INNER_UPDATE_INTERVAL;
		
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
		if (outdoor_update_delay > 0)
			return;
		
		outdoor_update_delay = OUTDOOR_UPDATE_INTERVAL;
		
		weather_info_t weather = { 0 };
		const char *weather_url = WEATHER_URL;
		const char *weather_http_response = esp_at_http_get(weather_url);
		if (weather_http_response == NULL)
		{
				printf("[WEATHER] http error!\n");
				outdoor_update_delay = SECONDS(5);
				return;
		}
		
		if (!parse_seniverse_response(weather_http_response, &weather))
		{
				printf("[WEATHER] parse failed!\n");
				outdoor_update_delay = SECONDS(5);
				return;
		}
		
		if (memcmp(&last_weather, &weather, sizeof(weather_info_t)) == 0)
				return;
		
		memcpy(&last_weather, &weather, sizeof(weather_info_t));
		printf("[WEATHER] %s, %s, %.1f\n", weather.city, weather.weather, weather.temperature);
		main_page_redraw_outdoor_temperature(weather.temperature);
		main_page_redraw_outdoor_weather_icon(weather.weather_code);
}

void main_loop(void)
{	
		time_sync();
		wifi_update();
		time_update();
		inner_update();
		outdoor_update();
}


