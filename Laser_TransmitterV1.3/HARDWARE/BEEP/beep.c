#include "sys.h" 
#include "beep.h"



//蜂鸣器 IO初始化
void BEEP_Init(void)
{
 	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	

	GPIO_InitStructure.GPIO_Pin = BEEP_PIN;				 //上电脚PB1端口配置
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	GPIO_Init(BEEP_PORT, &GPIO_InitStructure);					 //根据设定参数初始化
	BEEP_CLOSE;	///上电先给初始化为低电平
}


void BEEP_OUTPUT(void)
{
	GPIO_WriteBit(BEEP_PORT,BEEP_PIN,(BitAction)(1-GPIO_ReadOutputDataBit(BEEP_PORT,BEEP_PIN)));
}

