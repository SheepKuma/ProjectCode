#include "sys.h" 
#include "mytimer2.h"
#include "mytimer.h"
#include "beep.h"
#include "delay.h"


static void Timer2_Config(unsigned short arr, unsigned short psc);

unsigned char Beep_Cnt = 0;//���Ʒ���������ļ���λ


void MyTimer2_Init(void)
{
	Timer2_Config(200-1, 72-1);///250usһ��
	Beep_Cnt = 0;//���Ʒ���������ļ���λ
}


static void Timer2_Config(unsigned short arr, unsigned short psc)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); //ʱ��ʹ��
	TIM_DeInit(TIM2); 
	//��ʱ��TIM3��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);//�����ʱ����״̬��־λ���粶��״̬λ����ʱ��������־λ�ȡ���ֹ��λ�󣨰��¸�λ����,�����Զ�ִ��һ���жϣ���ʼ����ͽ��и����жϣ�
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE ); //ʹ��ָ����TIM2�ж�,��������ж�
	TIM_Cmd(TIM2, DISABLE);  //�Ȳ�������ʱ��������Ҫ�õ�BEEPʱ�ٿ���
	
	//TODO:��ȷ���ж����ȼ�
	//�ж����ȼ�NVIC����
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  //TIM3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//2  //��ռ���ȼ�2��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;//2  //�����ȼ�2��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);  //��ʼ��NVIC�Ĵ���

}



void MyTimer2_Close(void)
{
	TIM_Cmd(TIM2, DISABLE);///�رն�ʱ��2
	BEEP_CLOSE;
}

//�����ϵ�����������һ��(20ms)
void BEEP_PowerOn(void)
{
	TIM_Cmd(TIM2, ENABLE);  //�ȿ�����ʱ��
	CreatTimer(T_BEEP_PowerON,MyTimer2_Close,20,T_START_STA);//�ϵ���������20ms
}

//���ڹػ�ǰ����������һ��(300ms)
void BEEP_PowerOff(void)
{
	TIM_Cmd(TIM2, ENABLE);  //�ȿ�����ʱ��
	CreatTimer(T_BEEP,MyTimer2_Close,300,T_START_STA);//�ػ�ǰ��������300ms
}

//����BLE�����豸ָ�����������500ms
void BEEP_FindDev(void)
{
	TIM_Cmd(TIM2, ENABLE);  //�ȿ�����ʱ��
	CreatTimer(T_BEEP_FIND_DEV,MyTimer2_Close,500,T_START_STA);//�ػ�ǰ��������300ms
}

//ѵ����ʼʱ��������
void TrainStart_BeepCnt(void)
{
	static unsigned char num = 0;
	
	TIM_Cmd(TIM2, DISABLE);///�رն�ʱ��2
	BEEP_CLOSE;
	if(Beep_Cnt < 5)
	{
		num ^= 1;
		Beep_Cnt++;
		if(num == 0)
		{
			TIM_Cmd(TIM2, ENABLE);///������ʱ��2
		}
		else
		{
			TIM_Cmd(TIM2, DISABLE);///�رն�ʱ��2
			BEEP_CLOSE;
		}
		ResetTimer(T_BEEP_TRAINSTART,T_START_STA);
	}
}
//ѵ����ʼʱ��������
void BEEP_TrainStart(void)
{
	Beep_Cnt = 0;//�Ƚ���������
	TIM_Cmd(TIM2, ENABLE);  //�ȿ�����ʱ��
	CreatTimer(T_BEEP_TRAINSTART,TrainStart_BeepCnt,100,T_START_STA);//�ϵ���������200ms
}

//ѵ������ʱ��������
void TrainEnd_BeepCnt(void)
{
	static unsigned char num = 0;
	
	TIM_Cmd(TIM2, DISABLE);///�رն�ʱ��2
	BEEP_CLOSE;
	if(Beep_Cnt < 11)
	{
		num ^= 1;
		Beep_Cnt++;
		if(num == 0)
		{
			TIM_Cmd(TIM2, ENABLE);///������ʱ��2
		}
		else
		{
			TIM_Cmd(TIM2, DISABLE);///�رն�ʱ��2
			BEEP_CLOSE;
		}
		ResetTimer(T_BEEP_TRAINEND,T_START_STA);
	}
}
//ѵ������ʱ��������
void BEEP_TrainEnd(void)
{
	Beep_Cnt = 0;//�Ƚ���������
	TIM_Cmd(TIM2, ENABLE);  //�ȿ�����ʱ��
	CreatTimer(T_BEEP_TRAINEND,TrainEnd_BeepCnt,100,T_START_STA);
}

//�͵���ʱ��������������
void LowBattery_BeepCnt(void)
{
	static unsigned char num = 0;
	
	TIM_Cmd(TIM2, DISABLE);///�رն�ʱ��2
	BEEP_CLOSE;
	if(Beep_Cnt < 7)
	{
		num ^= 1;
		Beep_Cnt++;
		if(num == 0)
		{
			TIM_Cmd(TIM2, ENABLE);///������ʱ��2
		}
		else
		{
			TIM_Cmd(TIM2, DISABLE);///�رն�ʱ��2
			BEEP_CLOSE;
		}
		ResetTimer(T_BEEP_LOWBATTERY,T_START_STA);
	}
}
//δ��ʼ����ʱ���͵�����������
void Beep_LowBattery(void)
{
	Beep_Cnt = 0;//�Ƚ���������
	TIM_Cmd(TIM2, ENABLE);  //�ȿ�����ʱ��
	CreatTimer(T_BEEP_LOWBATTERY,LowBattery_BeepCnt,100,T_START_STA);
}

//���ģʽ�µķ�������1S
void BEEP_Pair(void)
{
	TIM_Cmd(TIM2, ENABLE);///�رն�ʱ��2
	CreatTimer(T_BEEP_PAIR,MyTimer2_Close,1000,T_START_STA);//������Ժ��������1S
}




void TIM2_IRQHandler(void) 
{
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)		//���TIM2�����жϷ������
	{
			BEEP_OUTPUT();
			TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  //���TIMx�����жϱ�־ 
	}
}


