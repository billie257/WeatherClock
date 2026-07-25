#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static const char *cwstate_response = 
"AT+CWSTATE?\r\n"
"+CWSTATE:2,\"vivo S30 Pro mini\"\r\n"
"\r\n"
"OK\r\n";

static const char *cwjap_response =
"AT+CWJAP?\r\n"
"+CWJAP:\"vivo S30 Pro mini\",\"a2:9e:3a:34:2b:c7\",1,-13,0,1,3,0,1\r\n"
"\r\n"
"OK\r\n";

static const char *seniverse_response =
"AT+HTTPCLIENT=2,1,\"https://api.seniverse.com/v3/weather/now.json?key=SQkuN5K6MY0peI79_&location=WS10730EM8EV&language=en&unit=c\",,,2\r\n"
"+HTTPCLIENT:279,{\"results\":[{\"location\":{\"id\":\"WS10730EM8EV\",\"name\":\"Shenzhen\",\"country\":\"CN\",\"path\":\"Shenzhen,Shenzhen,Guangdong,China\",\"timezone\":\"Asia/Shanghai\",\"timezone_offset\":\"+08:00\"},\"now\":{\"text\":\"Light rain\",\"code\":\"13\",\"temperature\":\"25\"},\"last_update\":\"2026-07-06T20:04:09+08:00\"}]}\r\n"
"\r\n"
"OK\r\n";

static const char *cipsntptime_response =
"AT+CIPSNTPTIME?\r\n"
"+CIPSNTPTIME:Tue Jul 7 17:57:57 2026\r\n"
"OK\r\n";

typedef struct
{
	char ssid[64];
	char bssid[18];
	int channel;
	int rssi;
	bool connected;
} wifi_info_t;

static bool parse_cwstate_response(const char *response, wifi_info_t *info)
{
	const char *p = strstr(response, "+CWSTATE:");
	if (p == NULL)
		return false;

	int wifi_state;
	if (sscanf(p, "+CWSTATE:%d,\"%[^\"]\"", &wifi_state, info->ssid) != 2)
		return false;
	
	info->connected = (wifi_state == 2);
	
	return true;
}

static bool parse_cwjap_response(const char *response, wifi_info_t *info)
{
	const char *p = strstr(response, "+CWJAP:");
	if (p == NULL)
		return false;
	
//	int parse_num = sscanf(p, "+CWJAP:\"%63[^\"]\", \"%17[^\"]\", %d, %d", info->ssid, info->bssid, &info->channel, &info->rssi);
//	printf("parse num: %d\n", parse_num);

	if (sscanf(p, "+CWJAP:\"%63[^\"]\", \"%17[^\"]\", %d, %d", info->ssid, info->bssid, &info->channel, &info->rssi) != 4)
		return false;
	
	return true;
}

static uint8_t month_str_to_num(const char *month_str)
{
	const char *months[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
							"Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
	for (uint8_t i = 0; i < 12; i++)
	{
		if (strcmp(month_str, months[i]) == 0)
		{
			return i + 1;
		}
	}
	return 0;
}

static uint8_t weekday_str_to_num(const char *weekday_str)
{
	const char *weekdays[] = {"Mon", "Tue", "Wed", "Thu", "Fri", "Sat", "Sun"};
	for (uint8_t i = 0; i < 7; i++)
	{
		if (strcmp(weekday_str, weekdays[i]) == 0)
		{
			return i + 1;  // 返回0-6，对应周日到周六
		}
	}
	return 1;  // 默认返回周日
}

typedef struct
{
	uint16_t year;
	uint8_t month;
	uint8_t day;
	uint8_t hour;
	uint8_t minute;
	uint8_t second; 
	uint8_t weekday;
} esp_date_time_t;

static bool parse_cipsntptime_response(const char *response, esp_date_time_t *date)
{
//	AT+CIPSNTPTIME?
//	+CIPSNTPTIME:Tue Jul 7 17:57:57 2026
//	OK
	char weekday_str[8];
	char month_str[4];
	const char *p = strstr(response, "+CIPSNTPTIME:");	
	if (sscanf(p, "+CIPSNTPTIME:%3s %3s %hhu %hhu:%hhu:%hhu %hu", 
			   weekday_str, month_str, &date->day, &date->hour, &date->minute, 
			   &date->second, &date->year) != 7)
		return false;
	
	date->weekday = weekday_str_to_num(weekday_str);
	date->month = month_str_to_num(month_str);
	
	return true;
}

typedef struct
{
	char city[32];
	char location[128];
	char weather[16];
	int weather_code;
	float temperature;
} weather_info_t;

static bool parse_seniverse_response(const char *response, weather_info_t *info)
{
	const char *p = strstr(response, "\"results\":");
	if (p == NULL)
		return false;
	
	const char *location_response = strstr(response, "\"location\":");
	if(location_response == NULL)
		return false;
	
	const char *location_name_response = strstr(location_response, "\"name\":");
	if (location_name_response)
	{
		sscanf(location_name_response, "\"name\": \"%31[^\"]\"", info->city);		
	}	
	const char *location_path_response = strstr(location_response, "\"path\":");
	if (location_path_response)
	{
		sscanf(location_path_response, "\"path\": \"%127[^\"]\"", info->location);		
	}
	
	const char *now_response = strstr(response, "\"now\":");
	if(now_response == NULL)
		return false;
	
	const char *now_text_response = strstr(now_response, "\"text\":");
	if (now_text_response)
	{
		sscanf(now_text_response, "\"text\": \"%15[^\"]\"", info->weather);		
	}
	
	const char *now_code_response = strstr(now_response, "\"code\":");
	if (now_code_response)
	{
		sscanf(now_code_response, "\"code\": \"%d\"", &info->weather_code);		
	}
	
	char temperature_str[16] = { 0 };
	const char *now_temperature_response = strstr(now_response, "\"temperature\":");
	if (now_temperature_response)
	{
		if (sscanf(now_temperature_response, "\"temperature\": \"%15[^\"]\"", temperature_str) == 1)
			info->temperature = atof(temperature_str);		
	}
	
	return true;
}

int main(void)
{
	wifi_info_t wifi_info = { 0 };
	if (!parse_cwstate_response(cwstate_response, &wifi_info))
	{
		printf("parse cwstate failed\n");
		return -1;
	}
	
	printf("[%s] %s\n", wifi_info.ssid, wifi_info.connected ? "connected" : "disconnected");
	
	if (!parse_cwjap_response(cwjap_response, &wifi_info))
	{
		printf("parse cwjap failed\n");
		return -1;
	}
	
	printf("[WIFI]\nssid: %s\nbssid: %s\nchannel: %d\nrssi: %d\n", wifi_info.ssid, wifi_info.bssid, wifi_info.channel, wifi_info.rssi);
	
	weather_info_t weather_info = { 0 };
	if (!parse_seniverse_response(seniverse_response, &weather_info))
	{
		printf("parse seniverse failed\n");
		return -1;
	}
	
	printf("[WEATHER]\ncity: %s\nlocation: %s\nweather: %s\ncode: %d\ntemperature: %.1f\n",
		   weather_info.city, weather_info.location,
		   weather_info.weather, weather_info.weather_code, weather_info.temperature);
	
	esp_date_time_t date;
	parse_cipsntptime_response(cipsntptime_response, &date);
	
	printf("%d %d %d %d %d %d %d\n", date.year, date.month,
		   date.day, date.hour, date.minute, date.second, date.weekday);
	
	return 0;
}
