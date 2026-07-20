#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "stm32f4xx.h"
#include "cpu_tick.h"
#include "esp_at.h"

#define ESP_AT_DEBUG    1

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof((arr)[0]))

typedef enum
{
    AT_ACK_NONE,
    AT_ACK_OK,
    AT_ACK_ERROR,
    AT_ACK_BUSY,
    AT_ACK_READY,
} at_ack_t;

typedef struct
{
    at_ack_t ack;
    const char *string;
} at_ack_match_t;

static const at_ack_match_t at_ack_matches[] = 
{
    {AT_ACK_OK, "OK\r\n"},
    {AT_ACK_ERROR, "ERROR\r\n"},
    {AT_ACK_BUSY, "busy p…\r\n"},
    {AT_ACK_READY, "ready\r\n"},
};

static char *rxline;
static char rxbuf[1024];
static uint32_t rxlen;
static at_ack_t rxack;
static SemaphoreHandle_t at_ack_semaphore;

static bool esp_at_write_command(const char *command, uint32_t timeout);
static void esp_at_usart_write(const char *data);
static bool esp_at_wait_boot(uint32_t timeout);
static bool	esp_at_wait_ready(uint32_t timeout);

static void esp_at_io_init(void)
{
		GPIO_PinAFConfig(GPIOA, GPIO_PinSource2, GPIO_AF_USART2);
		GPIO_PinAFConfig(GPIOA, GPIO_PinSource3, GPIO_AF_USART2);

		GPIO_InitTypeDef GPIO_InitStructure;
		GPIO_StructInit(&GPIO_InitStructure);
		GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2 | GPIO_Pin_3;
		GPIO_Init(GPIOA, &GPIO_InitStructure);
}

static void esp_at_usart_init(void)
{
		USART_InitTypeDef USART_InitStructure;
		USART_StructInit(&USART_InitStructure);

		USART_InitStructure.USART_BaudRate = 115200u;
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
		USART_InitStructure.USART_Parity = USART_Parity_No;
		USART_InitStructure.USART_StopBits = USART_StopBits_1;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;		
			
		USART_Init(USART2, &USART_InitStructure);  
		USART_DMACmd(USART2, USART_DMAReq_Tx, ENABLE);
		USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);
		USART_Cmd(USART2, ENABLE);
}

static void esp_at_dma_init(void)
{
		DMA_InitTypeDef DMA_InitStruct;
		DMA_StructInit(&DMA_InitStruct);
		DMA_InitStruct.DMA_Channel = DMA_Channel_4;
		DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)&USART2->DR;
		DMA_InitStruct.DMA_DIR = DMA_DIR_MemoryToPeripheral;
		DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
		DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
		DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
		DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
		DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
		DMA_InitStruct.DMA_Priority = DMA_Priority_Medium;
		DMA_InitStruct.DMA_FIFOMode = DMA_FIFOMode_Enable;
		DMA_InitStruct.DMA_FIFOThreshold = DMA_FIFOThreshold_Full;
		DMA_InitStruct.DMA_MemoryBurst = DMA_MemoryBurst_INC8;
		DMA_InitStruct.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;

		DMA_Init(DMA1_Stream6, &DMA_InitStruct);
}

static void esp_at_int_init(void)
{
		NVIC_InitTypeDef NVIC_InitStruct;
		NVIC_InitStruct.NVIC_IRQChannel = USART2_IRQn;
		NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 5;
		NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
		NVIC_Init(&NVIC_InitStruct);
		NVIC_SetPriority(USART2_IRQn, 5);
}

static void esp_at_lowlevel_init(void)
{
		esp_at_usart_init();
		esp_at_dma_init();
	  esp_at_int_init();
		esp_at_io_init();
}

bool esp_at_init(void)
{	
		at_ack_semaphore = xSemaphoreCreateBinary();
		configASSERT(at_ack_semaphore);
	
		esp_at_lowlevel_init();
	
	if (!esp_at_wait_boot(3000))
			return false;
	if (!esp_at_write_command("AT+RESTORE\r\n", 2000))
			return false;
	if (!esp_at_wait_ready(3000))
			return false;
	
	return true;
}

static void esp_at_usart_write(const char *data)
{
		uint32_t len = strlen(data);
	
		DMA1_Stream6->M0AR = (uint32_t)data;
		DMA1_Stream6->NDTR = len;
		
		DMA_ClearFlag(DMA1_Stream6, DMA_FLAG_TCIF6);
		DMA_Cmd(DMA1_Stream6, ENABLE);		
	
//		while(data && *data)
//		{
//				while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//				USART_SendData(USART2, *data++);
//		}
	
//		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//		USART_SendData(USART2, '\r');
//		while (USART_GetFlagStatus(USART2, USART_FLAG_TXE) == RESET);
//		USART_SendData(USART2, '\n');
}

static at_ack_t match_internal_ack(const char *str)
{
		for (uint32_t i = 0; i < ARRAY_SIZE(at_ack_matches); i++)
		{
				if (strcmp(str, at_ack_matches[i].string) == 0)
					return at_ack_matches[i].ack;
		}
		
		return AT_ACK_NONE;
}

static at_ack_t esp_at_usart_wait_receive(uint32_t timeout)
{
		rxlen = 0;
		rxline = rxbuf;
		bool acked = xSemaphoreTake(at_ack_semaphore, pdMS_TO_TICKS(timeout)) == pdPASS;
	  return acked ? rxack : AT_ACK_NONE;
}

static bool	esp_at_wait_ready(uint32_t timeout)
{
		return esp_at_usart_wait_receive(timeout) == AT_ACK_READY;
}

static bool esp_at_write_command(const char *command, uint32_t timeout)
{
#if ESP_AT_DEBUG
		printf("[DEBUG] Send: %s\n", command);
#endif
		esp_at_usart_write(command);
		at_ack_t ack = esp_at_usart_wait_receive(timeout);
	
#if ESP_AT_DEBUG
	printf("[DEBUG] Response: \n%s\n", rxbuf);
#endif
	
	return ack == AT_ACK_OK;
}

static const char *esp_at_get_reponse(void)
{
		return rxbuf;
}

static bool esp_at_wait_boot(uint32_t timeout)
{
		for (int t = 0; t < timeout; t += 100)
		{
				if (esp_at_write_command("AT\r\n", 100))
					return true;
		}
		return false;
}

bool esp_at_wifi_init(void)
{
		return esp_at_write_command("AT+CWMODE=1\r\n", 2000);
}

bool esp_at_connect_wifi(const char *ssid, const char *pwd, const char *mac)
{
		if (ssid == NULL || pwd == NULL)
				return false;
		char *cmd = rxbuf;
		int len = snprintf(cmd, sizeof(rxbuf), "AT+CWJAP=\"%s\",\"%s\"\r\n", ssid, pwd);
		if (mac)
				snprintf(cmd + len, sizeof(rxbuf) - len, ",\"%s\"", mac);
		return esp_at_write_command(cmd, 5000);
}

static bool parse_cwstate_response(const char *response, esp_wifi_info_t *info)
{
//AT+CWSTATE?
//+CWSTATE:2,"vivo S30 Pro mini"

//OK
	const char *p = strstr(response, "+CWSTATE:");
	if (p == NULL)
		return false;

	int wifi_state;
	if (sscanf(p, "+CWSTATE:%d,\"%[^\"]\"", &wifi_state, info->ssid) != 2)
		return false;
	
	info->connected = (wifi_state == 2);
	
	return true;
}

static bool parse_cwjap_response(const char *response, esp_wifi_info_t *info)
{
//AT+CWJAP?
//+CWJAP:"vivo S30 Pro mini","a2:9e:3a:34:2b:c7",1,-13,0,1,3,0,1

//OK
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

bool esp_at_get_wifi_info(esp_wifi_info_t *info)
{
		if (!esp_at_write_command("AT+CWSTATE?\r\n", 2000))
				return false;
		
		if (!parse_cwstate_response(esp_at_get_reponse(), info))
				return false;
		
		if (info->connected == true)
		{
			if (!esp_at_write_command("AT+CWJAP?\r\n", 2000))
				return false;
		
			if (!parse_cwjap_response(esp_at_get_reponse(), info))
				return false;
		}
		
		return true;
}

bool wifi_is_connected(void)
{
		esp_wifi_info_t info;
		if (esp_at_get_wifi_info(&info))
			return info.connected;
		
		return false;
}

bool esp_at_sntp_init(void)
{
		if (!esp_at_write_command("AT+CIPSNTPCFG=1,8\r\n", 2000))
				return false;
		
		return true;
}

bool esp_at_sntp_get_time(esp_date_time_t *date)
{
	 if (!esp_at_write_command("AT+CIPSNTPTIME?\r\n", 2000))
				return false;
	 
	 if (!parse_cipsntptime_response(esp_at_get_reponse(), date))
			return false;
		
		return true;	
}

const char *esp_at_http_get(const char *url)
{
//A://api.seniverse.com/v3/weather/now.json?key=SQkuN5K6MY0peI79_&location=WS10730EM8EV&language=en&unit=c",,,2
//+HTTPCLIENT:279,{"results":[{"location":{"id":"WS10730EM8EV","name":"Shenzhen","country":"CN","path":"Shenzhen,Shenzhen,Guangdong,China","timezone":"Asia/Shanghai","timezone_offset":"+08:00"},"now":{"text":"Light rain","code":"13","temperature":"25"},"last_update":"2026-07-06T20:04:09+08:00"}]}

//OK
	
		char *txbuf = rxbuf;
		snprintf(txbuf, sizeof(rxbuf), "AT+HTTPCLIENT=2,1,\"%s\",,,2\r\n", url);
		bool ret = esp_at_write_command(txbuf, 5000);
		return ret ? esp_at_get_reponse() : NULL;
}

void USART2_IRQHandler(void)
{	
		if (USART_GetITStatus(USART2, USART_IT_RXNE) == SET)
		{
				if (rxlen < sizeof(rxbuf) - 1)
				{	
						rxbuf[rxlen++] = USART_ReceiveData(USART2);
						if (rxbuf[rxlen - 1] == '\n')
						{
								rxbuf[rxlen] = '\0';
								at_ack_t ack = match_internal_ack(rxline);

								if (ack != AT_ACK_NONE)
								{
									  rxack = ack;
										BaseType_t pxHigherPriorityTaskWoken;
										xSemaphoreGiveFromISR(at_ack_semaphore, &pxHigherPriorityTaskWoken);
										portYIELD_FROM_ISR(pxHigherPriorityTaskWoken);
								}				
								rxline = rxbuf + rxlen;
						}			
				}			
				USART_ClearITPendingBit(USART2, USART_IT_RXNE);
		}		
}
