#include "sys.h" 
#include "led.h"



//LED IO初始化
void LED_Init(void)
{
 	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	

	//状态指示灯，输出高电平时小灯点亮
	GPIO_InitStructure.GPIO_Pin = LED_PIN;				 //上电脚PB1端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	GPIO_Init(LED_PORT, &GPIO_InitStructure);					 //根据设定参数初始化
	GPIO_SetBits(LED_PORT,LED_PIN);
	
	//设置START引脚,控制芯片上电下电
	GPIO_InitStructure.GPIO_Pin  = START_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //设置成强推挽输出
 	GPIO_Init(START_PORT, &GPIO_InitStructure);//初始化
}
 
