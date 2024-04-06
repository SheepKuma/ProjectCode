#ifndef _MYTIMER_H_
#define _MYTIMER_H_


typedef enum{
	T_KEY,
	T_TIMED_SHOOT,//��ʱ���ģʽ
	T_ADC,
	T_BEEP,//�ػ�ǰ300ms��һ��
	T_BEEP_PAIR,
	T_APP,//����Ӧ�ò���ش������ѯ����˵���
	T_APP_REFRESH_GUNNUM,//������ѯ�Ƿ���Ҫˢ���䵯��
	T_EXIT,//����������ⷢ��ı�־λ
	T_BEEP_TRAINSTART,//ѵ����ʼ
	T_BEEP_TRAINEND,//ѵ������
	T_STEPPER_CLOSE,//�رղ������
	T_IR_CALIBRATE,//У׼ģʽ
	T_BEEP_LOWBATTERY,//δ��ʼ����ʱ�͵�������
	T_BEEP_PowerON,//�ϵ�ʱ20ms����һ��
	T_BEEP_FIND_DEV,//�����豸����������500ms
	//T_PAIRKEY,//�����ϵ�3S���������
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
