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

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA |RCC_APB2Periph_ADC1	, ENABLE);	  //ʹ��ADC1ͨ��ʱ��
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);   //����ADC��Ƶ����6 72M/6=12,ADC���ʱ�䲻�ܳ���14M

	//PA1 ��Ϊģ��ͨ����������                         
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_1;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AIN;		//ģ����������
	GPIO_Init(GPIOA, &GPIO_InitStructure);	

	ADC_DeInit(ADC1);  //��λADC1
	ADC_InitStructure.ADC_Mode = ADC_Mode_Independent;	//ADC����ģʽ:ADC1��ADC2�����ڶ���ģʽ
	ADC_InitStructure.ADC_ScanConvMode = DISABLE;	//ģ��ת�������ڵ�ͨ��ģʽ
	ADC_InitStructure.ADC_ContinuousConvMode = DISABLE;	//ģ��ת�������ڵ���ת��ģʽ
	ADC_InitStructure.ADC_ExternalTrigConv = ADC_ExternalTrigConv_None;	//ת��������������ⲿ��������
	ADC_InitStructure.ADC_DataAlign = ADC_DataAlign_Right;	//ADC�����Ҷ���
	ADC_InitStructure.ADC_NbrOfChannel = 1;	//˳����й���ת����ADCͨ������Ŀ
	ADC_Init(ADC1, &ADC_InitStructure);	//����ADC_InitStruct��ָ���Ĳ�����ʼ������ADCx�ļĴ���   

	ADC_Cmd(ADC1, ENABLE);	//ʹ��ָ����ADC1
	ADC_ResetCalibration(ADC1);	//ʹ�ܸ�λУ׼  
	while(ADC_GetResetCalibrationStatus(ADC1));	//�ȴ���λУ׼����
	ADC_StartCalibration(ADC1);	 //����ADУ׼
	while(ADC_GetCalibrationStatus(ADC1));	 //�ȴ�У׼����
//	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������

	CreatTimer(T_ADC,ADC_Cycle,5000,T_START_STA);//5000--5S���һ�ε���//10000--10S���һ�ε���//30000--30S���һ�ε���
	ADC_Percent = 0;//��ʼ��Ϊ0
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
			Gun_DevInfo_Reply();//�ϱ��Կ�ǹе�豸״̬
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
	{///����3.2V���÷��������У�ͬʱ�ϱ��Կ��豸״̬
		current_percent = 0;
		//BEEP_PowerOff();
		//Gun_DevInfo_Reply();//�ϱ��Կ�ǹе�豸״̬
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



//���ADCֵ
//ch:ͨ��ֵ 0~3
static unsigned short Get_Adc(unsigned char ch)   
{
  	//����ָ��ADC�Ĺ�����ͨ����һ�����У�����ʱ��
	ADC_RegularChannelConfig(ADC1, ch, 1, ADC_SampleTime_239Cycles5 );	//ADC1,ADCͨ��,����ʱ��Ϊ239.5����	  			    
  
	ADC_SoftwareStartConvCmd(ADC1, ENABLE);		//ʹ��ָ����ADC1�����ת����������	
	 
	while(!ADC_GetFlagStatus(ADC1, ADC_FLAG_EOC ));//�ȴ�ת������

	return ADC_GetConversionValue(ADC1);	//�������һ��ADC1�������ת�����
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

//�ϵ�������
void ADC_PowerON(void)
{
	unsigned char i = 0;
	
	for(i=0;i<3;i++)
	{
		ADC_Proc();
		delay_ms(10);
	}
}

//��ȡ�����ٷֱ�
unsigned char Get_Battery(void)
{
	return ADC_Percent;
}
