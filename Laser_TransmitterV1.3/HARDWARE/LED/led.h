#ifndef __LED_H_
#define __LED_H_
#include "sys.h"




#define  LED_PORT  			GPIOB
#define  LED_PIN   			GPIO_Pin_1

#define  START_PORT			GPIOB
#define  START_PIN			GPIO_Pin_0

#define  STA_LED 				PBout(1)// PB1
#define  START_KEY 			PBout(0)// PB0



void LED_Init(void);//≥ı ºªØ

#endif

