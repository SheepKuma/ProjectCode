#include "sys.h"
#include "led.h"
#include "delay.h"
#include "key.h"
#include "usart.h"
#include "ir.h"
#include "app.h"
#include "exit.h"
#include "mytimer.h"
#include "adc.h"
#include "ble_usart.h"
#include "oled.h"
#include "oled_bmp.h"
#include "motor_ms3111s.h"
#include "stepper_motor.h"
#include "mytimer2.h"
#include "beep.h"


 int main(void)
 {
	 SystemInit();
	 delay_init();
	 NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //设置NVIC中断分组2:2位抢占优先级，2位响应优先级

	 LED_Init();
	 delay_ms(150);
	 START_KEY = 1;///上电
	 
	 Timer_Init();
	 BLE_Usart3_Init();
	 TypeC_Usart1_Init();
	 Motor_Ms3111s_Init();
	 Key_Init();
	 BEEP_Init();
	 MyTimer2_Init();
	 Adc_Init();
	 APP_Init();
	 //Exti_Init();
	 Ir_Init();
	 
	//检查电量和鸣叫蜂鸣器
   Machine_UnlockGun();//机械开锁
   BEEP_PowerOn();
   delay_ms(20);
	 //放在开锁之后，防止上电时开锁行为触发霍尔传感器
	 Exti_Init();
	 //改为APP配对
	 //Set_PairKey();
	 //CreatTimer(T_PAIRKEY,Clear_PairKey,3000,T_START_STA);//刚开机的前3S内允许配对

	 while(1)
	 {
		 if(BLE_RX_Flag)//查询蓝牙有无信息
		 {
			 BLE_Proc();
		 }
		 if(TypeC_RX_Flag)//查询Type-C有无信息
		 {
			 TypeC_Proc();
		 }
		 if(Timed_ShootFlag)//如果当前处于定时射击模式
		 {
			 if(HALL_KEY == 1)//如果扳机松开了，那就关闭定时射击
			 {
				 delay_ms(10);
				 if(HALL_KEY == 1)
				 {
					 CtrlTimer(T_TIMED_SHOOT, T_STOP_STA);//关闭定时射击
					 Timed_ShootFlag = 0;
				 }
			 }
		 }
		 if(GunLock_Flag)
		 {
			 //确认扳机松开了，再锁枪
			 if(HALL_KEY == 1)
			 {
				 delay_ms(10);
				 if(HALL_KEY == 1)
				 {
					 Machine_LockGun();//锁枪
					 GunLock_Flag = 0;
				 }
			 }
		 }
		 
	 }
	 
 }

