#ifndef _MYTIMER_H_
#define _MYTIMER_H_


typedef enum{
	T_KEY,
	T_TIMED_SHOOT,//定时射击模式
	T_ADC,
	T_BEEP,//关机前300ms响一声
	T_BEEP_PAIR,
	T_APP,//用于应用层相关处理的轮询，如菜单等
	T_APP_REFRESH_GUNNUM,//用来轮询是否需要刷新射弹数
	T_EXIT,//用于清除激光发射的标志位
	T_BEEP_TRAINSTART,//训练开始
	T_BEEP_TRAINEND,//训练结束
	T_STEPPER_CLOSE,//关闭步进电机
	T_IR_CALIBRATE,//校准模式
	T_BEEP_LOWBATTERY,//未开始比赛时低电量报警
	T_BEEP_PowerON,//上电时20ms短响一声
	T_BEEP_FIND_DEV,//查找设备蜂鸣器鸣叫500ms
	//T_PAIRKEY,//用于上电3S内允许配对
	T_SUM,
}TIMER_ID_TYPEDEF;

typedef enum{
	T_FAIL,
	T_SUCCESS,
}TIMER_RESULT_TYPEDEF;

typedef enum{
	T_STA_INVAILD,
	T_STOP_STA,
	T_START_STA,
}TIMER_STATE_TYPEDEF;

typedef struct{
	TIMER_STATE_TYPEDEF State;
	unsigned short Period;
	unsigned short CurrCount;
	void (*callback)(void);
}TimerPtr_Typdef;




void Timer_Init(void);
void CreatTimer(TIMER_ID_TYPEDEF id, void (*proc)(void), unsigned short period, TIMER_STATE_TYPEDEF sta);
void Change_TimerPeriod(TIMER_ID_TYPEDEF id, unsigned short period);
unsigned short Query_TimerPeriod(TIMER_ID_TYPEDEF id);
TIMER_RESULT_TYPEDEF CtrlTimer(TIMER_ID_TYPEDEF id, TIMER_STATE_TYPEDEF sta);
TIMER_STATE_TYPEDEF GetTimerSta(TIMER_ID_TYPEDEF id);
TIMER_RESULT_TYPEDEF 	DeleteTimer(TIMER_ID_TYPEDEF id);
TIMER_RESULT_TYPEDEF ResetTimer(TIMER_ID_TYPEDEF id, TIMER_STATE_TYPEDEF sta);




#endif
