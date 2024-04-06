#include "sys.h" 
#include "exit.h"
#include "ir.h"
#include "delay.h"
#include "mytimer.h"

static void Exit_GPIO_Config(void);
static void Exit_Config(void);

//激光发射标志位，=0 可以发送，=1不能发送，用来控制每段时间只能发送一次
volatile unsigned char Shoot_Flag = 0;
volatile unsigned char Timed_ShootFlag = 0;

void Exti_Init(void)
{
	Exit_GPIO_Config();
	Exit_Config();
	Shoot_Flag = 0;
	CreatTimer(T_EXIT,Reset_ShootFlag,100,T_STOP_STA);///控制每100ms只发射一次激光
}

static void Exit_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);//使能PB时钟

	//霍尔传感器信号输出脚PB9
	GPIO_InitStructure.GPIO_Pin  = HALL_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;///设置成上拉输入
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
 	GPIO_Init(HALL_PROT, &GPIO_InitStructure);//初始化

}

static void Exit_Config(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
 	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);	//使能复用功能时钟

	//GPIOB9 中断线以及中断初始化配置   下降沿触发
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource9);

	EXTI_InitStructure.EXTI_Line=EXTI_Line9;	
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	 	//根据EXTI_InitStruct中指定的参数初始化外设EXTI寄存器

	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;			//使能对应的外部中断通道
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//抢占优先级0， 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;					//子优先级0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//使能外部中断通道
	NVIC_Init(&NVIC_InitStructure);
}

//清零激光发射标志位
void Reset_ShootFlag(void)
{
	Shoot_Flag = 0;
}

//外部中断9服务程序
void EXTI9_5_IRQHandler(void)
{
	if(HALL_KEY == 0)
	{
		delay_ms(10);
		if(HALL_KEY == 0)
		{
			if(!Shoot_Flag)
			{
				Ir_Shot();//触发一次激光发射
				
				Shoot_Flag = 1;//标志位置起，不再触发激光
				ResetTimer(T_EXIT, T_START_STA);//200ms后再清除标志位

				if(Query_GunMode() == Timed_ShootMode)
				{
					ResetTimer(T_TIMED_SHOOT, T_START_STA);//开启定时射击
					Timed_ShootFlag = 1;//置起标志位
				}
			}
		}
	}
	EXTI_ClearITPendingBit(EXTI_Line9);  //清除LINE9上的中断标志位  
}




