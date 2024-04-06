#include "sys.h" 
#include "led.h"



//LED IO��ʼ��
void LED_Init(void)
{
 	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	

	//״ָ̬ʾ�ƣ�����ߵ�ƽʱС�Ƶ���
	GPIO_InitStructure.GPIO_Pin = LED_PIN;				 //�ϵ��PB1�˿�����
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
	GPIO_Init(LED_PORT, &GPIO_InitStructure);					 //�����趨������ʼ��
	GPIO_SetBits(LED_PORT,LED_PIN);
	
	//����START����,����оƬ�ϵ��µ�
	GPIO_InitStructure.GPIO_Pin  = START_PIN;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; //���ó�ǿ�������
 	GPIO_Init(START_PORT, &GPIO_InitStructure);//��ʼ��
}
 
