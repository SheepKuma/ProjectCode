#include "sys.h" 
#include "adc.h"
#include "delay.h"
#include "led.h"
#include "mytimer.h"
#include "oled.h"
#include "usart.h"
#include "beep.h"
#include "app.h"
#include "mytimer2.h"
#include "ir.h"

static unsigned short Get_Adc(unsigned char ch);
static unsigned short Get_Adc_Average(unsigned char ch, unsigned char times);

unsigned char ADC_Percent = 0;

void  Adc_Init(void)
{
	ADC_InitTypeDef ADC_InitStructure; 
	GPIO_InitTypeDef GPIO_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |RCC_APB2Periph_ADC1	, ENABLE);	  //使能ADC1通道时钟
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);   //设置ADC分频因子6 72M/6=12,ADC最大时间不能超过14M

	//PA1 作为模拟通道输入引脚                         
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		//模拟输入引脚
	GPIO_Init(GPIOA, &GPIO_InitStructure);	

	ADC_DeInit(ADC1);  //复位ADC1
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	//ADC工作模式:ADC1和ADC2工作在独立模式
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;	//模数转换工作在单通道模式
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;	//模数转换工作在单次转换模式
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//转换由软件而不是外部触发启动
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//ADC数据右对齐
	ADC_InitStructure.ADC_NbrOfChannel = 1;	//顺序进行规则转换的ADC通道的数目
	ADC_Init(ADC1, &ADC_InitStructure);	//根据ADC_InitStruct中指定的参数初始化外设ADCx的寄存器   

	ADC_Cmd(ADC1, ENABLE);	//使能指定的ADC1
	ADC_ResetCalibration(ADC1);	//使能复位校准  
	while(ADC_GetResetCalibrationStatus(ADC1));	//等待复位校准结束
	ADC_StartCalibration(ADC1);	 //开启AD校准
	while(ADC_GetCalibrationStatus(ADC1));	 //等待校准结束
//	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//使能指定的ADC1的软件转换启动功能

	CreatTimer(T_ADC,ADC_Cycle,5000,T_START_STA);//5000--5S检查一次电量//10000--10S检查一次电量//30000--30S检查一次电量
	ADC_Percent = 0;//初始化为0
}

void ADC_Cycle(void)
{
	//unsigned char i = 0;
	//unsigned char Percent = 0;
	
	/*for(i=0;i<3;i++)
	{
		ADC_Proc();
		Percent += ADC_Percent;
		delay_ms(10);
	}
	ADC_Percent = Percent / 3;*/
	ADC_Proc();
	if(ADC_Percent <= 25)
	{
		if(Query_TrainSta() == Train_OffSta)
		{
			Beep_LowBattery();
		}
	}
	if(ADC_Percent == 0)
	{
		if(Query_TrainSta() == Train_OnSta)
		{
			Gun_DevInfo_Reply();//上报显控枪械设备状态
		}
	}
	ResetTimer(T_ADC,T_START_STA);
}


void ADC_Proc(void)
{
	unsigned short AdcVal;
	float temp;
	static unsigned char last_percent = 0;
	static unsigned char current_percent = 0;
	static unsigned char cnt = 0;
	
	AdcVal = Get_Adc_Average(ADC_Channel_1,10);
	temp = ((float)AdcVal * (3.3/4096)) * 3;
	
	if(temp >= 4.2)
	{
		current_percent = 100;
	}
	else if(temp >= 3.8)
	{
		current_percent = 75 * temp - 215;
	}
	else if(temp >= 3.6)
	{
		current_percent = 225 * temp - 785;
	}
	else if(temp > 3.2)
	{
		current_percent = 62.5 * temp - 200;
	}
	else
	{///低于3.2V就让蜂鸣器鸣叫，同时上报显控设备状态
		current_percent = 0;
		//BEEP_PowerOff();
		//Gun_DevInfo_Reply();//上报显控枪械设备状态
	}
	
	if((last_percent < (current_percent-1)) || (last_percent > (current_percent+1)))
	{
		cnt++;
	}
	else
	{
		cnt = 0;
	}
	if(cnt >= 3)
	{
		cnt = 0;
		last_percent = ADC_Percent = current_percent;
		if((Query_MenuPosition() == DESKTOP_MENU_POS) && (!Query_ClrScreenFlag()))
		{
			Set_ClrScreenFlag();
		}
	}

}



//获得ADC值
//ch:通道值 0~3
static unsigned short Get_Adc(unsigned char ch)   
{
  	//设置指定ADC的规则组通道，一个序列，采样时间
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADC通道,采样时间为239.5周期	  			    
  
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//使能指定的ADC1的软件转换启动功能	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//等待转换结束

	return ADC_GetConversionValue(ADC1);	//返回最近一次ADC1规则组的转换结果
}

static unsigned short Get_Adc_Average(unsigned char ch, unsigned char times)
{
	unsigned int temp_val=0;
	unsigned char t;
	for(t=0;t<times;t++)
	{
		temp_val+=Get_Adc(ch);
		delay_ms(5);
	}
	return temp_val/times;
} 	 

//上电后检查电量
void ADC_PowerON(void)
{
	unsigned char i = 0;
	
	for(i=0;i<3;i++)
	{
		ADC_Proc();
		delay_ms(10);
	}
}

//获取电量百分比
unsigned char Get_Battery(void)
{
	return ADC_Percent;
}
