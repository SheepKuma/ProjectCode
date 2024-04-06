#include "sys.h" 
#include "mytimer.h"


volatile TimerPtr_Typdef TimerList[T_SUM];

static void Timer_Config(unsigned short arr, unsigned short psc);
static void TimerHandle(void);



void Timer_Init(void){

	Timer_Config(1000-1, 72-1);//1ms
	
	for(unsigned char i = 0; i < T_SUM; i++){
		TimerList[i].State = T_STOP_STA;
		TimerList[i].CurrCount = 0;
		TimerList[i].Period = 0;
		TimerList[i].callback = 0;
	}
	
}


static void Timer_Config(unsigned short arr, unsigned short psc){
	
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //ʱ��ʹ��
	TIM_DeInit(TIM3); 
	//��ʱ��TIM3��ʼ��
	TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //����ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
 
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);//�����ʱ����״̬��־λ���粶��״̬λ����ʱ��������־λ�ȡ���ֹ��λ�󣨰��¸�λ����,�����Զ�ִ��һ���жϣ���ʼ����ͽ��и����жϣ�
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //ʹ��ָ����TIM3�ж�,��������ж�
	TIM_Cmd(TIM3, ENABLE);  //ʹ��TIMx		
	
	//�ж����ȼ�NVIC����
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3�ж�
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;//2  //��ռ���ȼ�1��
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;//2  //�����ȼ�0��
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQͨ����ʹ��
	NVIC_Init(&NVIC_InitStructure);  //��ʼ��NVIC�Ĵ���
}

void CreatTimer(TIMER_ID_TYPEDEF id, void (*proc)(void), unsigned short period, TIMER_STATE_TYPEDEF sta){

	TimerList[id].State = sta;
	TimerList[id].CurrCount = 0;
	TimerList[id].Period = period;
	TimerList[id].callback = proc;
	
}

//���Ķ�ʱ��������
void Change_TimerPeriod(TIMER_ID_TYPEDEF id, unsigned short period)
{
	TimerList[id].Period = period;
}

//��ѯ��ʱ��������
unsigned short Query_TimerPeriod(TIMER_ID_TYPEDEF id)
{
	return TimerList[id].Period;
}

///������������CurrCount���㣬���»ص�����������ִ��
TIMER_RESULT_TYPEDEF CtrlTimer(TIMER_ID_TYPEDEF id, TIMER_STATE_TYPEDEF sta){

	if(TimerList[id].callback){
		TimerList[id].State = sta;
		return T_SUCCESS;
	}
	else{
		return T_FAIL;
	}
	
}


TIMER_STATE_TYPEDEF GetTimerSta(TIMER_ID_TYPEDEF id){

	if(TimerList[id].callback){
		return TimerList[id].State;
	}
	else{
		return T_STA_INVAILD;
	}
	
}


TIMER_RESULT_TYPEDEF 	DeleteTimer(TIMER_ID_TYPEDEF id){

	if(TimerList[id].callback){
		TimerList[id].State = T_STOP_STA;
		TimerList[id].CurrCount = 0;
		TimerList[id].callback = 0;
		return T_SUCCESS;
	}
	else{
		return T_FAIL;
	}
	
}

///����������CurrCount���㣬�����ص������Ż����±�ִ��
TIMER_RESULT_TYPEDEF ResetTimer(TIMER_ID_TYPEDEF id, TIMER_STATE_TYPEDEF sta){

	if(TimerList[id].callback){
		TimerList[id].State = sta;
		TimerList[id].CurrCount = 0;
		
		return T_SUCCESS;
	}
	else{
		return T_FAIL;
	}
	
}




static void TimerHandle(void){
	
	for(unsigned char i = 0; i < T_SUM; i++){
		
		if((TimerList[i].callback) && (TimerList[i].State == T_START_STA)){
			TimerList[i].CurrCount++;
			if(TimerList[i].CurrCount >= TimerList[i].Period){
				TimerList[i].State = T_STOP_STA;
				TimerList[i].CurrCount = TimerList[i].CurrCount;
				TimerList[i].callback();
			}
		}
		
	}
	
}



void TIM3_IRQHandler(void)   //TIM3�ж�
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  //���TIM3�����жϷ������
		{
				TimerHandle();
				TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //���TIMx�����жϱ�־ 
		}
}





