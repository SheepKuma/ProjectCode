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
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC|RCC_APB2Periph_AFIO, ENABLE);	 //ʹ��PC�˿�ʱ��
	PWR_BackupAccessCmd(ENABLE);/* �����޸�RTC�ͺ󱸼Ĵ���*/
  RCC_LSEConfig(RCC_LSE_OFF); /* �ر��ⲿ����ʱ��,PC14+PC15����������ͨIO*/
  BKP_TamperPinCmd(DISABLE);  /* �ر����ּ�⹦��,PC13����������ͨIO*/

	///��ʼ��MS3111S��IN1�ţ�IN1��IN2������͵�ƽʱ��Ϊ����оƬ��sleep̬
	GPIO_InitStructure.GPIO_Pin = MS3111S_IN1_PIN;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;		
	GPIO_Init(MS3111S_IN1_PORT, &GPIO_InitStructure);					
	GPIO_ResetBits(MS3111S_IN1_PORT,MS3111S_IN1_PIN);				
	
	///��ʼ��MS3111S��IN2�ţ�IN1��IN2������͵�ƽʱ��Ϊ����оƬ��sleep̬
	GPIO_InitStructure.GPIO_Pin = MS3111S_IN2_PIN;				 
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_2MHz;		
	GPIO_Init(MS3111S_IN2_PORT, &GPIO_InitStructure);					
	GPIO_ResetBits(MS3111S_IN2_PORT,MS3111S_IN2_PIN);				
	
	PWR_BackupAccessCmd(DISABLE);/* ��ֹ�޸�RTC�ͺ󱸼Ĵ���*/
}


//��е��ǹ
void Machine_LockGun(void)
{
	MS3111S_IN1_OUT = 0;
	MS3111S_IN2_OUT = 1;
	
	delay_ms(5);
	//ɲ��
	MS3111S_IN1_OUT = 1;
	MS3111S_IN2_OUT = 1;
	delay_ms(5);
	//˯��
	MS3111S_IN1_OUT = 0;
	MS3111S_IN2_OUT = 0;
}

//��е����
void Machine_UnlockGun(void)
{
		MS3111S_IN1_OUT = 1;
		MS3111S_IN2_OUT = 0;
	
	//���Է���ֻҪ���ɲ���̫��չ���Ͳ���Ҫ̫�õ���ʱ�����5ms���У�10ms���ԣ��ʶ�20ms
	//��ʱ����ʱ˲ʱ����������180ma���ң�Ȼ��ܿ�ָ�������80ma����
		delay_ms(120);//20
		//ɲ��
		MS3111S_IN1_OUT = 1;
		MS3111S_IN2_OUT = 1;
		delay_ms(5);
		//˯��
		MS3111S_IN1_OUT = 0;
		MS3111S_IN2_OUT = 0;
}


