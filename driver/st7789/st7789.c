#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "stm32f4xx.h"
#include "cpu_tick.h"
#include "st7789.h"
#include "font.h"
#include "image.h"

// CLK    PA5
// MOSI   PA7
// RESET  PC0
// DC     PC1
// CS     PC2

#define RESET_PORT    GPIOC
#define RESET_PIN     GPIO_Pin_0
#define DC_PORT    		GPIOC
#define DC_PIN     		GPIO_Pin_1
#define CS_PORT    		GPIOC
#define CS_PIN     		GPIO_Pin_2

#define SCK_PORT      GPIOA
#define SCK_PIN       GPIO_Pin_5
#define MOSI_PORT     GPIOA
#define MOSI_PIN      GPIO_Pin_7

#define delay_us(x) cpu_delay_us(x)
#define delay_ms(x) cpu_delay_ms(x)

static void st7789_init_display(void);

void st7789_init(void)
{
		GPIO_InitTypeDef GPIO_InitStruct;
		GPIO_StructInit(&GPIO_InitStruct);
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
		GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_NOPULL;
		GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_5 | GPIO_Pin_7;
		GPIO_Init(GPIOA, &GPIO_InitStruct);
	
		GPIO_PinAFConfig(GPIOA, GPIO_PinSource5, GPIO_AF_SPI1);
		GPIO_PinAFConfig(GPIOA, GPIO_PinSource7, GPIO_AF_SPI1);
	
		GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStruct.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2;     
    GPIO_Init(GPIOC, &GPIO_InitStruct);
	
		// By default, raise the CS and RST levels
		GPIO_SetBits(CS_PORT, CS_PIN);
	  GPIO_SetBits(RESET_PORT, RESET_PIN);
	  GPIO_SetBits(DC_PORT, DC_PIN);
	
		SPI_InitTypeDef SPI_InitStruct;
		SPI_StructInit(&SPI_InitStruct);
		SPI_InitStruct.SPI_Direction = SPI_Direction_1Line_Tx;
		SPI_InitStruct.SPI_Mode = SPI_Mode_Master;
		SPI_InitStruct.SPI_DataSize = SPI_DataSize_8b;
		SPI_InitStruct.SPI_CPOL = SPI_CPOL_Low;
		SPI_InitStruct.SPI_CPHA = SPI_CPHA_1Edge;
		SPI_InitStruct.SPI_NSS = SPI_NSS_Soft;
		SPI_InitStruct.SPI_BaudRatePrescaler = SPI_BaudRatePrescaler_4;
		SPI_InitStruct.SPI_FirstBit = SPI_FirstBit_MSB;
		SPI_Init(SPI1, &SPI_InitStruct);	
		SPI_Cmd(SPI1, ENABLE);
		st7789_init_display();
}

static void st7789_write_register(uint8_t reg, uint8_t data[], uint16_t length)
{
		GPIO_ResetBits(CS_PORT, CS_PIN);
	  GPIO_ResetBits(DC_PORT, DC_PIN);		
		SPI_SendData(SPI1, reg);
		while (SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE) == RESET);	
		while (SPI_GetFlagStatus(SPI1, SPI_FLAG_BSY) != RESET);
		
		GPIO_SetBits(DC_PORT, DC_PIN);
		for (uint16_t i = 0; i < length; i++)
		{
			SPI_SendData(SPI1, data[i]);	
			while (!SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE));
		}
		while (SPI_GetFlagStatus(SPI1, SPI_FLAG_BSY) != RESET);
		
		GPIO_SetBits(CS_PORT, CS_PIN);
}

static void st7789_reset(void)
{
		GPIO_ResetBits(RESET_PORT, RESET_PIN);
		delay_us(20);
		GPIO_SetBits(RESET_PORT, RESET_PIN);
		delay_ms(120);
}

static void st7789_init_display(void)
{
		GPIO_SetBits(SCK_PORT, SCK_PIN);	
		st7789_reset();
		
		st7789_write_register(0x01, NULL, 0); //Software Reset
		delay_ms(120);
						
		st7789_write_register(0x11, NULL, 0); //Sleep Out
		delay_ms(120);               //DELAY120ms
	
		//-----------------------ST7789V Frame rate setting-----------------//
		//************************************************
		st7789_write_register(0x3A, (uint8_t[]){0x05}, 1); //65k mode
	
		st7789_write_register(0xC5, (uint8_t[]){0x1A}, 1); //VCOM
	
		st7789_write_register(0x36, (uint8_t[]){0x00}, 1); // screen display direction setting
	
		//-------------ST7789V Frame rate setting-----------//

		st7789_write_register(0xb2, (uint8_t[]){0x05, 0x05, 0x00, 0x33, 0x33}, 5);		//Porch Setting

		st7789_write_register(0xb7, (uint8_t[]){0x05}, 1); //Gate Control 12.2v   -10.43v
		//--------------ST7789V Power setting---------------//
		
		st7789_write_register(0xBB, (uint8_t[]){0x3F}, 1); //VCOM

		st7789_write_register(0xC0, (uint8_t[]){0x2c}, 1); //Power control

		st7789_write_register(0xC2, (uint8_t[]){0x01}, 1); //VDV and VRH Command Enable

		st7789_write_register(0xC3, (uint8_t[]){0x0F}, 1); //VRH Set 4.3+( vcom+vcom offset+vdv)

		st7789_write_register(0xC4, (uint8_t[]){0x20}, 1); //VDV Set 0v

		st7789_write_register(0xC6, (uint8_t[]){0X01}, 1); //Frame Rate Control in Normal Mode 111Hz

		st7789_write_register(0xD0, (uint8_t[]){0xA4, 0xA1}, 2); //Power Control 1

		st7789_write_register(0xE8, (uint8_t[]){0x03}, 1);  //Power Control 1

		st7789_write_register(0xE9, (uint8_t[]){0x09, 0x09, 0x08}, 3); //Equalize time control
		
		//---------------ST7789V gamma setting-------------//

		st7789_write_register(0xE0, (uint8_t[]){0xD0,0x05,0x09,0x09,0x08,0x14,0x28,0x33,0x3F,0x07,0x13,0x14,0x28,0x30}, 14);//Set Gamma

		st7789_write_register(0XE1, (uint8_t[]){0xD0,0x05,0x09,0x09,0x08,0x03,0x24,0x32,0x32,0x3B,0x14,0x13,0x28,0x2F}, 14);//Set Gamma

		st7789_write_register(0x20, NULL, 0); // Inverse display
		delay_ms(120);       
		st7789_write_register(0x29, NULL, 0); //turn on inverse display
		
		st7789_fill_color(0,0,ST7789_WIDTH - 1, ST7789_HEIGHT - 1, BLACK);
		
}

static bool in_sreen_range(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd)
{
		if (xSta >= ST7789_WIDTH || ySta >= ST7789_HEIGHT)
				return false;
		if (xEnd >= ST7789_WIDTH || yEnd >= ST7789_HEIGHT)
				return false;
		if (xSta > xEnd || ySta > yEnd)
				return false;
		return true;
}

static void st7789_set_window_and_prepare_gram(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd)
{
		st7789_write_register(0x2A, (uint8_t[]){(xSta >> 8) & 0xff, xSta & 0xff, (xEnd >> 8) & 0xff, xEnd & 0xff}, 4); // Column address set
		st7789_write_register(0x2B, (uint8_t[]){(ySta >> 8) & 0xff, ySta & 0xff, (yEnd >> 8) & 0xff, yEnd & 0xff}, 4); // Row address set
		st7789_write_register(0x2C, NULL, 0); //Memory write
}

void st7789_fill_color(uint16_t xSta, uint16_t ySta, uint16_t xEnd, uint16_t yEnd, uint16_t color)
{
    if (!in_sreen_range(xSta,ySta,xEnd,yEnd))
			return;
					
		st7789_set_window_and_prepare_gram(xSta, ySta, xEnd, yEnd);
		
		uint8_t color_data[2] = {(color >> 8) & 0xff, color & 0xff};
		
		GPIO_ResetBits(CS_PORT, CS_PIN);
		GPIO_SetBits(DC_PORT, DC_PIN);
		
		uint32_t point_count = (xEnd - xSta + 1) * (yEnd - ySta + 1);
		 
		for (uint32_t i = 0; i < point_count; i++)
		{
			SPI_SendData(SPI1, color_data[0]);
			while (SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE) == RESET);
			SPI_SendData(SPI1, color_data[1]);
			while (SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE) == RESET);
		}
		while (SPI_GetFlagStatus(SPI1, SPI_FLAG_BSY) != RESET);
		
		GPIO_SetBits(CS_PORT, CS_PIN);		
}

static void st7789_draw_font(uint16_t x, uint16_t y, uint16_t width, uint16_t height, const uint8_t *model, uint16_t color, uint16_t bg_color)
{
		uint16_t bytes_per_row = (width + 7) / 8;
		uint8_t color_data[2] = {(color >> 8) & 0xff, color & 0xff};
		uint8_t bg_color_data[2] = {(bg_color >> 8) & 0xff, bg_color & 0xff};
		st7789_set_window_and_prepare_gram(x, y, x + width - 1, y + height - 1);
		
		GPIO_ResetBits(CS_PORT, CS_PIN);
		GPIO_SetBits(DC_PORT, DC_PIN);
			
		for (uint16_t row = 0; row < height; row++)
		{
			const uint8_t *row_data = model + row * bytes_per_row;
			for (uint16_t col = 0; col < width; col++)
			{
				uint8_t pixel = row_data[col / 8] & (1 << (7 - col % 8));
				
				if (pixel)
				{
						SPI_SendData(SPI1, color_data[0]);
						while (SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE) == RESET);
						SPI_SendData(SPI1, color_data[1]);
						while (SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE) == RESET);
				}		
				else
				{
						SPI_SendData(SPI1, bg_color_data[0]);
						while (SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE) == RESET);
						SPI_SendData(SPI1, bg_color_data[1]);
						while (SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE) == RESET);
				}
			}
		}
		while (SPI_GetFlagStatus(SPI1, SPI_FLAG_BSY) != RESET);
		
		GPIO_SetBits(CS_PORT, CS_PIN);	
}

static const uint8_t *ascii_get_model(const char ch, const font_t *font)
{
		uint16_t bytes_per_row = (font->size / 2 + 7) / 8;
		uint16_t bytes_per_char = font->size * bytes_per_row;
		if (font->ascii_map)
		{
				const char *map = font->ascii_map;
				do
				{
						if (*map == ch)
						{
							 return font->ascii_model + (map - font->ascii_map) * bytes_per_char;
						}
				} while(*(++map) != '\0');	
		}	
		else
		{
				return font->ascii_model + (ch - ' ') * bytes_per_char;
		}

		return NULL;
}

static void st7789_write_ascii(uint16_t x, uint16_t y, char ch, uint16_t color, uint16_t bg_color, const font_t *font)
{
		if (font == NULL)
				return;
		
		if (ch < 0x20 || ch > 0x7E)
				return;
		
		uint16_t fheight = font->size, fwidth = font->size / 2;
	
	  if (!in_sreen_range(x, y, x + fwidth - 1, y + fheight - 1))
			return;
		
		const uint8_t *model = ascii_get_model(ch, font);
		if (model != NULL)
				st7789_draw_font(x, y, fwidth, fheight, model, color, bg_color);
}

static void st7789_write_chinese(uint16_t x, uint16_t y, const char *ch, uint16_t color, uint16_t bg_color, const font_t *font)
{
		if (ch == NULL || font == NULL)
				return;
		
		uint16_t fheight = font->size, fwidth = font->size;
	
	  if (!in_sreen_range(x, y, x + fwidth - 1, y + fheight - 1))
			return;
		
		const font_chinese_t *c = font->chinese;
		for (; c->name != NULL; c++)
		{
			 if (strcmp(c->name, ch) == 0)
					break;
		}
		if (c->name == NULL)
			return;
			
		st7789_draw_font(x, y, fwidth, fheight, c->model, color, bg_color);	
}

static bool is_gb2312(char ch)
{
		return ((unsigned char)ch >= 0xA1 && (unsigned char)ch <= 0xF7);
}

//static int utf8_char_length(const char *str)
//{
//		if ((*str & 0x80) == 0) return 1; // 1 byte
//		if ((*str & 0xE0) == 0xC0) return 2; // 2 byte
//		if ((*str & 0xF0) == 0xE0) return 3; // 3 byte
//		if ((*str & 0xF8) == 0xF0) return 4; // 4 byte
//		return -1; // Invalid UTF-8
//}

void st7789_write_string(int16_t x, uint16_t y, const char *str, uint16_t color, uint16_t bg_color, const font_t *font)
{
		while(*str)
		{
			//int len = utf8_char_length(*str);
			int len = is_gb2312(*str) ? 2 : 1;
			if (len <= 0)
			{
				 str++;
				 continue;
			}
			else if (len == 1)
			{
					st7789_write_ascii(x, y, *str, color, bg_color, font);
					str++;
					x += font->size / 2;				
			}
			else
			{
					char ch[5]; // utf8 num of byte is 4 , plus \0 is 5
					strncpy(ch, str, len);
					st7789_write_chinese(x, y, ch, color, bg_color, font);
					str+=len;
					x += font->size;				
			}
		}
}

void st7789_draw_image(uint16_t x, uint16_t y, const image_t *image)
{
		if (x >= ST7789_WIDTH || y >= ST7789_HEIGHT ||
				x + image->width - 1 >= ST7789_WIDTH || y + image->height - 1 >= ST7789_HEIGHT)
				return;
		
		st7789_set_window_and_prepare_gram(x, y, x + image->width - 1, y + image->height - 1);
		
		GPIO_ResetBits(CS_PORT, CS_PIN);
		GPIO_SetBits(DC_PORT, DC_PIN);
		uint32_t size = image->width * image->height * 2; // len of data   ps:RGB565
		const uint8_t *data = image->data;			
		for (uint32_t i = 0; i < size; i += 2)  // RGB565 240*240*2=115200 > 65535 so use uint32_t
		{			
				SPI_SendData(SPI1, data[i + 1]);			
				while (SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE) == RESET);
				SPI_SendData(SPI1, data[i]);			
				while (SPI_GetFlagStatus(SPI1, SPI_FLAG_TXE) == RESET);
		}		
		while (SPI_GetFlagStatus(SPI1, SPI_FLAG_BSY) != RESET);		
		GPIO_SetBits(CS_PORT, CS_PIN);	
}
