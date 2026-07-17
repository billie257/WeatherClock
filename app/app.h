#ifndef __APP_H__
#define __APP_H__

#define APP_VERSION "v1.0"
#define WIFI_SSID "vivo S30 Pro mini"
#define WIFI_PASSWD "bec1234567"
#define WEATHER_URL "https://api.seniverse.com/v3/weather/now.json?key=SQkuN5K6MY0peI79_&location=WS10730EM8EV&language=en&unit=c";

void wifi_init(void);
void wifi_wait_connect(void);
void main_loop_init(void);
void main_loop(void);

#endif /* __APP_H__ */
