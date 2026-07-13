#include <stdbool.h>
#include <stddef.h>
#include "key_desc.h"
#include "key.h"

void key_init(key_desc_t key)
{
	  GPIO_InitTypeDef GPIO_InitStruct;
	
	  GPIO_StructInit(& GPIO_InitStruct);
	  GPIO_InitStruct.GPIO_Mode = GPIO_Mode_IN;
	  GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	  GPIO_InitStruct.GPIO_Speed = GPIO_Speed_25MHz;		  
	  
	  EXTI_InitTypeDef EXTI_InitStruct;
	  EXTI_StructInit(&EXTI_InitStruct);
	  EXTI_InitStruct.EXTI_Mode = EXTI_Mode_Interrupt;
	  EXTI_InitStruct.EXTI_Trigger = EXTI_Trigger_Falling;
	  EXTI_InitStruct.EXTI_LineCmd = ENABLE;
	  	
	  NVIC_InitTypeDef NVIC_InitStruct;
	  NVIC_InitStruct.NVIC_IRQChannelPreemptionPriority = 5;
		NVIC_InitStruct.NVIC_IRQChannelSubPriority = 0;
		NVIC_InitStruct.NVIC_IRQChannelCmd = ENABLE;
				
		// KEY init
		GPIO_InitStruct.GPIO_Pin = key->pin;
		GPIO_Init(key->port, &GPIO_InitStruct);
		SYSCFG_EXTILineConfig(key->exti_port_src, key->exti_pin_src);
		EXTI_InitStruct.EXTI_Line = key->exti_line;
		EXTI_Init(&EXTI_InitStruct);
		NVIC_InitStruct.NVIC_IRQChannel = key->irqn;
		NVIC_Init(&NVIC_InitStruct);	
}

bool key_read(key_desc_t key)
{
		return GPIO_ReadInputDataBit(key->port, key->pin) == Bit_SET;
}

void key_press_callback_register(key_desc_t key, key_func_t func)
{
		key->func = func; 
}
