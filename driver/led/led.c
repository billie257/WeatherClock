#include <stdbool.h>
#include <stdlib.h>
#include "led_desc.h"
#include "led.h"

void led_init(led_desc_t led)
{
	if (led == NULL)
			return;
		GPIO_InitTypeDef  GPIO_InitStructure;
		GPIO_StructInit(&GPIO_InitStructure);	
		
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
		GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
		GPIO_InitStructure.GPIO_Speed = GPIO_High_Speed;
		GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
	
		GPIO_InitStructure.GPIO_Pin = led->Pin;
		GPIO_Init(led->Port, &GPIO_InitStructure);
	
		GPIO_WriteBit(led->Port, led->Pin, led->OffBit);	
}

void led_set(led_desc_t led, bool onoff)
{
	  if (led == NULL)
			return;
		GPIO_WriteBit(led->Port, led->Pin, onoff ? led->OnBit : led->OffBit);
}

void led_on(led_desc_t led)
{
	if (led == NULL)
			return;
		GPIO_WriteBit(led->Port, led->Pin, led->OnBit);
}

void led_off(led_desc_t led)
{
	if (led == NULL)
			return;
		GPIO_WriteBit(led->Port, led->Pin, led->OffBit);
}
