#ifndef __KEY_H
#define __KEY_H	 
#include "sys.h"
 	 

#define  SET_KEY_PORT		GPIOC
#define  SET_KEY_PIN		GPIO_Pin_15//GPIO_Pin_12

#define  POWER_PORT			GPIOC
#define  POWER_PIN			GPIO_Pin_5

#define  POWER_KEY  		GPIO_ReadInputDataBit(POWER_PORT,POWER_PIN)

///���������ѯʱ��
#define  KEY_SCANT_TICK            1//10//10MS
///��������
#define  KEY_SCANTIME               2//5//20//20MS
///��������ʱ��
#define  KEY_PRESS_LONG_TIME       200//200//150//200//200//2000//2S
///�����������ʱ��
#define  KEY_PRESS_CONTINUE_TIME   300//150//150MS

typedef enum
{
	KEY_SET,
	KEY_POWER,
	KEY_NUM,
}KEY_TYPEDEF;  //��������

// ����������
typedef enum{
	KEY_STEP_WAIT,			//�ȴ�����
	KEY_STEP_CLICK,				//��������
	KEY_STEP_LONG_PRESS,				//����
	KEY_STEP_CONTINUOUS_PRESS,  			//��������
}KEY_STEP_TYPEDEF;

typedef enum
{	
	KEY_IDLE,       	 		 							//��������
	KEY_CLICK,          								//����ȷ��
	KEY_CLICK_RELEASE,            			//�����ͷ�
	KEY_LONG_PRESS,			   						 	//����ȷ��
	KEY_LONG_PRESS_CONTINUOUS,					//��������
	KEY_LONG_PRESS_RELEASE							//�����ͷ�
	 
}KEY_EVENT_TYPEDEF;

typedef enum
{
	KEY_IDLE_VAL = 0,	//0
	KEY_SET_CLICK,		//1
	KEY_SET_CLICK_RELEASE,		//2
	KEY_SET_LONG_PRESS,			//3
	KEY_SET_LONG_PRESS_CONTINUOUS,	//4
	KEY_SET_LONG_PRESS_RELEASE,		//5
	
	KEY_POWER_CLICK,		//6
	KEY_POWER_CLICK_RELEASE,		//7
	KEY_POWER_LONG_PRESS,			//8
	KEY_POWER_LONG_PRESS_CONTINUOUS,	//9
	KEY_POWER_LONG_PRESS_RELEASE,		//10
	
}KEY_VALUE_TYPEDEF;


typedef void (*KeyEvent_CallBack_t)(KEY_VALUE_TYPEDEF keys);


void Key_Init(void);
void KeyScan_CBSRegister(KeyEvent_CallBack_t pCBS);
void Key_Proc(void);	


#endif
