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

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能
	TIM_DeInit(TIM3); 
	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ClearFlag(TIM3, TIM_FLAG_Update);//清除定时器的状态标志位，如捕获状态位，定时器触发标志位等。防止复位后（按下复位键）,立即自动执行一次中断（初始化完就进行更新中断）
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断
	TIM_Cmd(TIM3, ENABLE);  //使能TIMx		
	
	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 1;//2  //先占优先级1级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;//2  //从优先级0级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器
}

void CreatTimer(TIMER_ID_TYPEDEF id, void (*proc)(void), unsigned short period, TIMER_STATE_TYPEDEF sta){

	TimerList[id].State = sta;
	TimerList[id].CurrCount = 0;
	TimerList[id].Period = period;
	TimerList[id].callback = proc;
	
}

//更改定时器的周期
void Change_TimerPeriod(TIMER_ID_TYPEDEF id, unsigned short period)
{
	TimerList[id].Period = period;
}

//查询定时器的周期
unsigned short Query_TimerPeriod(TIMER_ID_TYPEDEF id)
{
	return TimerList[id].Period;
}

///这个函数不会把CurrCount清零，导致回调函数来不及执行
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

///这个函数会把CurrCount清零，这样回调函数才会重新被执行
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



void TIM3_IRQHandler(void)   //TIM3中断
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  //检查TIM3更新中断发生与否
		{
				TimerHandle();
				TIM_ClearITPendingBit(TIM3, TIM_IT_Update);  //清除TIMx更新中断标志 
		}
}





