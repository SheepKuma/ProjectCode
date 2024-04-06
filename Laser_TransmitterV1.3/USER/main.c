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
	 NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //����NVIC�жϷ���2:2λ��ռ���ȼ���2λ��Ӧ���ȼ�

	 LED_Init();
	 delay_ms(150);
	 START_KEY = 1;///�ϵ�
	 
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
	 
	//�����������з�����
   Machine_UnlockGun();//��е����
   BEEP_PowerOn();
   delay_ms(20);
	 //���ڿ���֮�󣬷�ֹ�ϵ�ʱ������Ϊ��������������
	 Exti_Init();
	 //��ΪAPP���
	 //Set_PairKey();
	 //CreatTimer(T_PAIRKEY,Clear_PairKey,3000,T_START_STA);//�տ�����ǰ3S���������

	 while(1)
	 {
		 if(BLE_RX_Flag)//��ѯ����������Ϣ
		 {
			 BLE_Proc();
		 }
		 if(TypeC_RX_Flag)//��ѯType-C������Ϣ
		 {
			 TypeC_Proc();
		 }
		 if(Timed_ShootFlag)//�����ǰ���ڶ�ʱ���ģʽ
		 {
			 if(HALL_KEY == 1)//�������ɿ��ˣ��Ǿ͹رն�ʱ���
			 {
				 delay_ms(10);
				 if(HALL_KEY == 1)
				 {
					 CtrlTimer(T_TIMED_SHOOT, T_STOP_STA);//�رն�ʱ���
					 Timed_ShootFlag = 0;
				 }
			 }
		 }
		 if(GunLock_Flag)
		 {
			 //ȷ�ϰ���ɿ��ˣ�����ǹ
			 if(HALL_KEY == 1)
			 {
				 delay_ms(10);
				 if(HALL_KEY == 1)
				 {
					 Machine_LockGun();//��ǹ
					 GunLock_Flag = 0;
				 }
			 }
		 }
		 
	 }
	 
 }

