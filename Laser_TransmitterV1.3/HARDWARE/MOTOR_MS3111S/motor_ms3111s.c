#include "sys.h"
#include "motor_ms3111s.h"
#include "delay.h"


static void ms3111s_gpio_config(void);


void Motor_Ms3111s_Init(void)
{
	ms3111s_gpio_config();
}

static void ms3111s_gpio_config(void)
{
	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO, ENABLE);	 //使能PC端口时钟
	PWR_BackupAccessCmd(ENABLE);/* 允许修改RTC和后备寄存器*/
  RCC_LSEConfig(RCC_LSE_OFF); /* 关闭外部低速时钟,PC14+PC15可以用作普通IO*/
  BKP_TamperPinCmd(DISABLE);  /* 关闭入侵检测功能,PC13可以用作普通IO*/

	///初始化MS3111S的IN1脚，IN1和IN2都输出低电平时，为驱动芯片的sleep态
	GPIO_InitStructure.GPIO_Pin = MS3111S_IN1_PIN;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;		
	GPIO_Init(MS3111S_IN1_PORT, &GPIO_InitStructure);					
	GPIO_ResetBits(MS3111S_IN1_PORT,MS3111S_IN1_PIN);				
	
	///初始化MS3111S的IN2脚，IN1和IN2都输出低电平时，为驱动芯片的sleep态
	GPIO_InitStructure.GPIO_Pin = MS3111S_IN2_PIN;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;		
	GPIO_Init(MS3111S_IN2_PORT, &GPIO_InitStructure);					
	GPIO_ResetBits(MS3111S_IN2_PORT,MS3111S_IN2_PIN);				
	
	PWR_BackupAccessCmd(DISABLE);/* 禁止修改RTC和后备寄存器*/
}


//机械锁枪
void Machine_LockGun(void)
{
	MS3111S_IN1_OUT = 0;
	MS3111S_IN2_OUT = 1;
	
	delay_ms(5);
	//刹车
	MS3111S_IN1_OUT = 1;
	MS3111S_IN2_OUT = 1;
	delay_ms(5);
	//睡眠
	MS3111S_IN1_OUT = 0;
	MS3111S_IN2_OUT = 0;
}

//机械开锁
void Machine_UnlockGun(void)
{
		MS3111S_IN1_OUT = 1;
		MS3111S_IN2_OUT = 0;
	
	//测试发现只要弹簧不是太舒展，就不需要太久的延时，最短5ms不行，10ms可以，故定20ms
	//此时开锁时瞬时电流会增大到180ma左右，然后很快恢复正常的80ma左右
		delay_ms(120);//20
		//刹车
		MS3111S_IN1_OUT = 1;
		MS3111S_IN2_OUT = 1;
		delay_ms(5);
		//睡眠
		MS3111S_IN1_OUT = 0;
		MS3111S_IN2_OUT = 0;
}


