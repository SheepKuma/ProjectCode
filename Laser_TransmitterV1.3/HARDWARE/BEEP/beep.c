#include "sys.h" 
#include "beep.h"



//������ IO��ʼ��
void BEEP_Init(void)
{
 	GPIO_InitTypeDef  GPIO_InitStructure;
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	

	GPIO_InitStructure.GPIO_Pin = BEEP_PIN;				 //�ϵ��PB1�˿�����
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //�������
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO���ٶ�Ϊ50MHz
	GPIO_Init(BEEP_PORT, &GPIO_InitStructure);					 //�����趨������ʼ��
	BEEP_CLOSE;	///�ϵ��ȸ���ʼ��Ϊ�͵�ƽ
}


void BEEP_OUTPUT(void)
{
	GPIO_WriteBit(BEEP_PORT,BEEP_PIN,(BitAction)(1-GPIO_ReadOutputDataBit(BEEP_PORT,BEEP_PIN)));
}

