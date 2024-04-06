#include "sys.h" 
#include "exit.h"
#include "ir.h"
#include "delay.h"
#include "mytimer.h"

static void Exit_GPIO_Config(void);
static void Exit_Config(void);

//���ⷢ���־λ��=0 ���Է��ͣ�=1���ܷ��ͣ���������ÿ��ʱ��ֻ�ܷ���һ��
volatile unsigned char Shoot_Flag = 0;
volatile unsigned char Timed_ShootFlag = 0;

void Exti_Init(void)
{
	Exit_GPIO_Config();
	Exit_Config();
	Shoot_Flag = 0;
	CreatTimer(T_EXIT,Reset_ShootFlag,100,T_STOP_STA);///����ÿ100msֻ����һ�μ���
}

static void Exit_GPIO_Config(void)
{
	GPIO_InitTypeDef GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);//ʹ��PBʱ��

	//�����������ź������PB9
	GPIO_InitStructure.GPIO_Pin  = HALL_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;///���ó���������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
 	GPIO_Init(HALL_PROT, &GPIO_InitStructure);//��ʼ��

}

static void Exit_Config(void)
{
	EXTI_InitTypeDef EXTI_InitStructure;
 	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO,ENABLE);	//ʹ�ܸ��ù���ʱ��

	//GPIOB9 �ж����Լ��жϳ�ʼ������   �½��ش���
	GPIO_EXTILineConfig(GPIO_PortSourceGPIOB,GPIO_PinSource9);

	EXTI_InitStructure.EXTI_Line=EXTI_Line9;	
	EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;	
	EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;//EXTI_Trigger_Rising;
	EXTI_InitStructure.EXTI_LineCmd = ENABLE;
	EXTI_Init(&EXTI_InitStructure);	 	//����EXTI_InitStruct��ָ���Ĳ�����ʼ������EXTI�Ĵ���

	NVIC_InitStructure.NVIC_IRQChannel = EXTI9_5_IRQn;			//ʹ�ܶ�Ӧ���ⲿ�ж�ͨ��
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;	//��ռ���ȼ�0�� 
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;					//�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;								//ʹ���ⲿ�ж�ͨ��
	NVIC_Init(&NVIC_InitStructure);
}

//���㼤�ⷢ���־λ
void Reset_ShootFlag(void)
{
	Shoot_Flag = 0;
}

//�ⲿ�ж�9�������
void EXTI9_5_IRQHandler(void)
{
	if(HALL_KEY == 0)
	{
		delay_ms(10);
		if(HALL_KEY == 0)
		{
			if(!Shoot_Flag)
			{
				Ir_Shot();//����һ�μ��ⷢ��
				
				Shoot_Flag = 1;//��־λ���𣬲��ٴ�������
				ResetTimer(T_EXIT, T_START_STA);//200ms���������־λ

				if(Query_GunMode() == Timed_ShootMode)
				{
					ResetTimer(T_TIMED_SHOOT, T_START_STA);//������ʱ���
					Timed_ShootFlag = 1;//�����־λ
				}
			}
		}
	}
	EXTI_ClearITPendingBit(EXTI_Line9);  //���LINE9�ϵ��жϱ�־λ  
}




