#include "stm32f10x.h"
#include "key.h"
#include "sys.h" 
#include "delay.h"
#include "mytimer.h"
#include "usart.h"
								    
static void Key_Config(void);
static unsigned char Get_KeySet_Sta(void);
static unsigned char Get_KeyPower_Sta(void);

								    
KeyEvent_CallBack_t KeyScanCBS;

unsigned char (*GetKeysState[KEY_NUM])() = {Get_KeySet_Sta, Get_KeyPower_Sta};

unsigned char KeyStep[KEY_NUM];							//�����������
unsigned short KeyScanTime[KEY_NUM];				//ȥ����ʱ
unsigned short KeyPressLongTimer[KEY_NUM];	//������ʱ
unsigned short KeyContPressTimer[KEY_NUM];	//����������ʱ




void Key_Init(void)
{
	unsigned char i = 0;
	KeyScanCBS = 0;
	Key_Config();
	
	for(i=0; i<KEY_NUM; i++)
	{
		KeyStep[i] = KEY_STEP_WAIT;
		KeyScanTime[i] = KEY_SCANTIME;
		KeyPressLongTimer[i] = KEY_PRESS_LONG_TIME;
		KeyContPressTimer[i] = KEY_PRESS_CONTINUE_TIME;
	}
	
	CreatTimer(T_KEY,Key_Proc,10,T_START_STA);
}

//������ʼ������
static void Key_Config(void) //IO��ʼ��
{ 
	GPIO_InitTypeDef GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO,ENABLE);//ʹ��PB PC ���ý�ʱ��
	
	RCC_LSEConfig( RCC_LSE_OFF ); /* �ر��ⲿ����ʱ��,PC14+PC15����������ͨIO*/
	//���ð�������,����Ϊ�ߵ�ƽ  ///��IO��Ϊ�͵�ƽ���ʾ��������
	GPIO_InitStructure.GPIO_Pin  = SET_KEY_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //���ó���������
 	GPIO_Init(SET_KEY_PORT, &GPIO_InitStructure);//��ʼ��
	
	//����POWER����,��������Դ�Ƿ��µ�
	GPIO_InitStructure.GPIO_Pin  = POWER_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; //���ó�ǿ�������
 	GPIO_Init(POWER_PORT, &GPIO_InitStructure);//��ʼ��
}

void KeyScan_CBSRegister(KeyEvent_CallBack_t pCBS)
{
	if(KeyScanCBS == 0)
	{
		KeyScanCBS = pCBS;
	}
}

unsigned char keys;//�������ݰ���ֵ
void Key_Proc(void)
{
	unsigned char KeyState[KEY_NUM];
	unsigned char i = 0;
	
	for(i=0; i<KEY_NUM; i++)
	{
		keys = 0;

		KeyState[i] = GetKeysState[i]();
		switch(KeyStep[i])
		{
			case KEY_STEP_WAIT:		//�ȴ�����
				if(KeyState[i])
				{
					KeyStep[i] = KEY_STEP_CLICK;	
				}
			break;
			case KEY_STEP_CLICK:				//������������
				if(KeyState[i])
				{
					if(!(--KeyScanTime[i]))
					{
						KeyScanTime[i] = KEY_SCANTIME;
						KeyStep[i] = KEY_STEP_LONG_PRESS;
						//keys = i+1;										//��¼����ID��
						//state = KEY_CLICK;								//������������
						keys = (i*5)+1;		
					}
				}else
				{
					KeyScanTime[i] = KEY_SCANTIME;
					KeyStep[i] = KEY_STEP_WAIT;
				}
			break;
			case KEY_STEP_LONG_PRESS:			//��������
				if(KeyState[i])
				{	
					if(!(--KeyPressLongTimer[i]))
					{
						KeyPressLongTimer[i] = KEY_PRESS_LONG_TIME;
						KeyStep[i] = KEY_STEP_CONTINUOUS_PRESS;
						
						keys = (i*5)+3;								//����ȷ��
					 
					}
				}else
				{
					KeyPressLongTimer[i] = KEY_PRESS_LONG_TIME;
					KeyStep[i] = KEY_STEP_WAIT;
					keys = (i*5)+2;										//�����ͷ�
				}
			break;
			case KEY_STEP_CONTINUOUS_PRESS:
				if(KeyState[i])
				{
					if(!(--KeyContPressTimer[i]))
					{
						KeyContPressTimer[i] = KEY_PRESS_CONTINUE_TIME;
						keys = (i*5)+4;					//��������
					}
				}else
				{
					KeyStep[i] = KEY_STEP_WAIT;
					KeyContPressTimer[i] = KEY_PRESS_CONTINUE_TIME;
					keys = (i*5)+5;								//�����ͷ�
				}
				 
			break;
					
		}
		
		if(keys)
		{
			
			if(KeyScanCBS)
			{	 
				
				KeyScanCBS((KEY_VALUE_TYPEDEF)keys);
			}
		}
	}
	
	ResetTimer(T_KEY,T_START_STA);
}

static unsigned char Get_KeySet_Sta(void)
{
	//��IO��״̬Ϊ�͵�ƽ�����ʾ��������
	return (!GPIO_ReadInputDataBit(SET_KEY_PORT, SET_KEY_PIN));
}

static unsigned char Get_KeyPower_Sta(void)
{
	//��IO��״̬Ϊ�ߵ�ƽ�����ʾ��������
	return (GPIO_ReadInputDataBit(POWER_PORT, POWER_PIN));
}
