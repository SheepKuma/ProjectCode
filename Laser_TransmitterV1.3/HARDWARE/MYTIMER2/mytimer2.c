#include "sys.h" 
#include "mytimer2.h"
#include "mytimer.h"
#include "beep.h"
#include "delay.h"


static void Timer2_Config(unsigned short arr, unsigned short psc);

unsigned char Beep_Cnt = 0;//控制蜂鸣器节奏的计数位


void MyTimer2_Init(void)
{
	Timer2_Config(200-1, 72-1);///250us一次
	Beep_Cnt = 0;//控制蜂鸣器节奏的计数位
}


static void Timer2_Config(unsigned short arr, unsigned short psc)
{
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE); //时钟使能
	TIM_DeInit(TIM2); 
	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ClearFlag(TIM2, TIM_FLAG_Update);//清除定时器的状态标志位，如捕获状态位，定时器触发标志位等。防止复位后（按下复位键）,立即自动执行一次中断（初始化完就进行更新中断）
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE ); //使能指定的TIM2中断,允许更新中断
	TIM_Cmd(TIM2, DISABLE);  //先不开启定时器，等需要用到BEEP时再开启
	
	//TODO:待确认中断优先级
	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;//2  //抢占优先级2级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;//2  //从优先级2级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器

}



void MyTimer2_Close(void)
{
	TIM_Cmd(TIM2, DISABLE);///关闭定时器2
	BEEP_CLOSE;
}

//用于上电后蜂鸣器鸣叫一声(20ms)
void BEEP_PowerOn(void)
{
	TIM_Cmd(TIM2, ENABLE);  //先开启定时器
	CreatTimer(T_BEEP_PowerON,MyTimer2_Close,20,T_START_STA);//上电后蜂鸣器响20ms
}

//用于关机前蜂鸣器鸣叫一声(300ms)
void BEEP_PowerOff(void)
{
	TIM_Cmd(TIM2, ENABLE);  //先开启定时器
	CreatTimer(T_BEEP,MyTimer2_Close,300,T_START_STA);//关机前蜂鸣器响300ms
}

//用于BLE查找设备指令，蜂鸣器鸣叫500ms
void BEEP_FindDev(void)
{
	TIM_Cmd(TIM2, ENABLE);  //先开启定时器
	CreatTimer(T_BEEP_FIND_DEV,MyTimer2_Close,500,T_START_STA);//关机前蜂鸣器响300ms
}

//训练开始时短响三声
void TrainStart_BeepCnt(void)
{
	static unsigned char num = 0;
	
	TIM_Cmd(TIM2, DISABLE);///关闭定时器2
	BEEP_CLOSE;
	if(Beep_Cnt < 5)
	{
		num ^= 1;
		Beep_Cnt++;
		if(num == 0)
		{
			TIM_Cmd(TIM2, ENABLE);///开启定时器2
		}
		else
		{
			TIM_Cmd(TIM2, DISABLE);///关闭定时器2
			BEEP_CLOSE;
		}
		ResetTimer(T_BEEP_TRAINSTART,T_START_STA);
	}
}
//训练开始时短响三声
void BEEP_TrainStart(void)
{
	Beep_Cnt = 0;//先将计数清零
	TIM_Cmd(TIM2, ENABLE);  //先开启定时器
	CreatTimer(T_BEEP_TRAINSTART,TrainStart_BeepCnt,100,T_START_STA);//上电后蜂鸣器响200ms
}

//训练结束时短响六声
void TrainEnd_BeepCnt(void)
{
	static unsigned char num = 0;
	
	TIM_Cmd(TIM2, DISABLE);///关闭定时器2
	BEEP_CLOSE;
	if(Beep_Cnt < 11)
	{
		num ^= 1;
		Beep_Cnt++;
		if(num == 0)
		{
			TIM_Cmd(TIM2, ENABLE);///开启定时器2
		}
		else
		{
			TIM_Cmd(TIM2, DISABLE);///关闭定时器2
			BEEP_CLOSE;
		}
		ResetTimer(T_BEEP_TRAINEND,T_START_STA);
	}
}
//训练结束时短响六声
void BEEP_TrainEnd(void)
{
	Beep_Cnt = 0;//先将计数清零
	TIM_Cmd(TIM2, ENABLE);  //先开启定时器
	CreatTimer(T_BEEP_TRAINEND,TrainEnd_BeepCnt,100,T_START_STA);
}

//低电量时报警，短响四声
void LowBattery_BeepCnt(void)
{
	static unsigned char num = 0;
	
	TIM_Cmd(TIM2, DISABLE);///关闭定时器2
	BEEP_CLOSE;
	if(Beep_Cnt < 7)
	{
		num ^= 1;
		Beep_Cnt++;
		if(num == 0)
		{
			TIM_Cmd(TIM2, ENABLE);///开启定时器2
		}
		else
		{
			TIM_Cmd(TIM2, DISABLE);///关闭定时器2
			BEEP_CLOSE;
		}
		ResetTimer(T_BEEP_LOWBATTERY,T_START_STA);
	}
}
//未开始比赛时，低电量短响四声
void Beep_LowBattery(void)
{
	Beep_Cnt = 0;//先将计数清零
	TIM_Cmd(TIM2, ENABLE);  //先开启定时器
	CreatTimer(T_BEEP_LOWBATTERY,LowBattery_BeepCnt,100,T_START_STA);
}

//配对模式下的蜂鸣器响1S
void BEEP_Pair(void)
{
	TIM_Cmd(TIM2, ENABLE);///关闭定时器2
	CreatTimer(T_BEEP_PAIR,MyTimer2_Close,1000,T_START_STA);//长按配对后蜂鸣器响1S
}




void TIM2_IRQHandler(void) 
{
	if(TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)		//检查TIM2更新中断发生与否
	{
			BEEP_OUTPUT();
			TIM_ClearITPendingBit(TIM2, TIM_IT_Update);  //清除TIMx更新中断标志 
	}
}


