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

unsigned char KeyStep[KEY_NUM];							//按键检测流程
unsigned short KeyScanTime[KEY_NUM];				//去抖延时
unsigned short KeyPressLongTimer[KEY_NUM];	//长按延时
unsigned short KeyContPressTimer[KEY_NUM];	//连续长按延时




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

//按键初始化函数
static void Key_Config(void) //IO初始化
{ 
	GPIO_InitTypeDef GPIO_InitStructure;
 	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_AFIO,ENABLE);//使能PB PC 复用脚时钟
	
	RCC_LSEConfig( RCC_LSE_OFF ); /* 关闭外部低速时钟,PC14+PC15可以用作普通IO*/
	//设置按键引脚,正常为高电平  ///若IO口为低电平则表示按键按下
	GPIO_InitStructure.GPIO_Pin  = SET_KEY_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; //设置成下拉输入
 	GPIO_Init(SET_KEY_PORT, &GPIO_InitStructure);//初始化
	
	//设置POWER引脚,用来检测电源是否下电
	GPIO_InitStructure.GPIO_Pin  = POWER_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD; //设置成强推挽输出
 	GPIO_Init(POWER_PORT, &GPIO_InitStructure);//初始化
}

void KeyScan_CBSRegister(KeyEvent_CallBack_t pCBS)
{
	if(KeyScanCBS == 0)
	{
		KeyScanCBS = pCBS;
	}
}

unsigned char keys;//用来传递按键值
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
			case KEY_STEP_WAIT:		//等待按键
				if(KeyState[i])
				{
					KeyStep[i] = KEY_STEP_CLICK;	
				}
			break;
			case KEY_STEP_CLICK:				//按键单击按下
				if(KeyState[i])
				{
					if(!(--KeyScanTime[i]))
					{
						KeyScanTime[i] = KEY_SCANTIME;
						KeyStep[i] = KEY_STEP_LONG_PRESS;
						//keys = i+1;										//记录按键ID号
						//state = KEY_CLICK;								//按键单击按下
						keys = (i*5)+1;		
					}
				}else
				{
					KeyScanTime[i] = KEY_SCANTIME;
					KeyStep[i] = KEY_STEP_WAIT;
				}
			break;
			case KEY_STEP_LONG_PRESS:			//按键长按
				if(KeyState[i])
				{	
					if(!(--KeyPressLongTimer[i]))
					{
						KeyPressLongTimer[i] = KEY_PRESS_LONG_TIME;
						KeyStep[i] = KEY_STEP_CONTINUOUS_PRESS;
						
						keys = (i*5)+3;								//长按确认
					 
					}
				}else
				{
					KeyPressLongTimer[i] = KEY_PRESS_LONG_TIME;
					KeyStep[i] = KEY_STEP_WAIT;
					keys = (i*5)+2;										//单击释放
				}
			break;
			case KEY_STEP_CONTINUOUS_PRESS:
				if(KeyState[i])
				{
					if(!(--KeyContPressTimer[i]))
					{
						KeyContPressTimer[i] = KEY_PRESS_CONTINUE_TIME;
						keys = (i*5)+4;					//持续长按
					}
				}else
				{
					KeyStep[i] = KEY_STEP_WAIT;
					KeyContPressTimer[i] = KEY_PRESS_CONTINUE_TIME;
					keys = (i*5)+5;								//长按释放
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
	//若IO口状态为低电平，则表示按键按下
	return (!GPIO_ReadInputDataBit(SET_KEY_PORT, SET_KEY_PIN));
}

static unsigned char Get_KeyPower_Sta(void)
{
	//若IO口状态为高电平，则表示按键按下
	return (GPIO_ReadInputDataBit(POWER_PORT, POWER_PIN));
}
