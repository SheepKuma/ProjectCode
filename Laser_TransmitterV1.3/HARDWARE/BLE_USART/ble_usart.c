#include "sys.h"
#include "ble_usart.h"
#include "usart.h"	 
#include "app.h"
#include <string.h>


usart_rx_data BLE_RX={0};
unsigned char BEL_DebugBuff[USART_REC_LEN]={0};	

volatile unsigned char BLE_RX_Flag = 0;

static void BLE_Usart3_Config(u32 bound);
static void usart3_DMA_config(void);
static void Usart3_SendByte(unsigned char  Dat);


void BLE_Usart3_Init(void)
{
	BLE_Usart3_Config(115200);
	usart3_DMA_config();
	BLE_RX_Flag = 0;
}

static void BLE_Usart3_Config(u32 bound)
{	
	//GPIO�˿�����
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);	//ʹ��USART2��GPIOAʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART3, ENABLE);
	//USART3_TX   GPIOB.10
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10; //GPIOB.10
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//�����������
	GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��GPIOB.10

	//USART3_RX	  GPIOB.11
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_11;//GPIOB.11
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//��������
	GPIO_Init(GPIOB, &GPIO_InitStructure);//��ʼ��GPIOB.11

	USART_InitStructure.USART_BaudRate = bound;//���ڲ�����
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//�ֳ�Ϊ8λ���ݸ�ʽ
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//һ��ֹͣλ
	USART_InitStructure.USART_Parity = USART_Parity_No;//����żУ��λ
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//��Ӳ������������
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//�շ�ģʽ

	//USART ��ʼ������
	USART_Init(USART3, &USART_InitStructure); //��ʼ������3
  USART_ITConfig(USART3, USART_IT_IDLE, ENABLE);
	USART_DMACmd(USART3,USART_DMAReq_Rx,ENABLE);
  USART_Cmd(USART3, ENABLE);                    //ʹ�ܴ���3
	
	//Usart3 NVIC ����
	NVIC_InitStructure.NVIC_IRQChannel = USART3_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=2 ;//��ռ���ȼ�2
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;		//�����ȼ�0
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQͨ��ʹ��
	NVIC_Init(&NVIC_InitStructure);	//����ָ���Ĳ�����ʼ��VIC�Ĵ���
}

static void usart3_DMA_config(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//ʹ��DMA����
	DMA_DeInit(DMA1_Channel3);
	
	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART3->DR);  //DMA�������ַ
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)BLE_RX.rx_buf;  //DMA�ڴ����ַ
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;  //���ݴ��䷽��
	DMA_InitStructure.DMA_BufferSize = USART_REC_LEN;  //DMAͨ����DMA����Ĵ�С
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  //�����ַ�Ĵ�������
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //�ڴ��ַ�Ĵ�������
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  //���ݿ��Ϊ8λ
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; //���ݿ��Ϊ8λ
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  //����������ģʽ
	DMA_InitStructure.DMA_Priority = DMA_Priority_High; //DMAͨ�� xӵ�������ȼ� 
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  //DMAͨ��xû������Ϊ�ڴ浽�ڴ洫��
	DMA_Init(DMA1_Channel3, &DMA_InitStructure);  //����DMA_InitStruct��ָ���Ĳ�����ʼ��DMA��ͨ��

	DMA_Cmd(DMA1_Channel3,ENABLE);

	DMA_ITConfig(DMA1_Channel3,DMA_IT_TC,ENABLE);
}


static void Usart3_SendByte(unsigned char  Dat)
{
	USART_SendData(USART3, Dat);
	while(USART_GetFlagStatus (USART3 ,USART_FLAG_TXE )!=SET );
}


void BLE_TX(unsigned char *tx_buff,unsigned char len)
{
	unsigned char i = 0;

	for(;i<len; i++)
	{
		Usart3_SendByte(tx_buff[i]);//�򴮿�1��������
	}
	while(USART_GetFlagStatus(USART3,USART_FLAG_TC)!=SET);//�ȴ����ͽ���
	
}

void BLE_Proc(void)
{
//	BLE_TX(BLE_RX.rx_buf, BLE_RX.rx_len);///TEST
	unsigned char len = 0;
	
	len = BLE_RX.rx_len;
	memcpy(BEL_DebugBuff,BLE_RX.rx_buf,len);
	if((BEL_DebugBuff[0] == BLE_HEADER) && (BEL_DebugBuff[len-1] == BLE_TAIL)) 
	{
		BLE_DataHandle(BEL_DebugBuff, len);
	}
	
	BLE_RX_Flag = 0;
}

//����3�жϷ������
void USART3_IRQHandler(void)                	
{
	if(USART_GetITStatus(USART3, USART_IT_IDLE) != RESET)  
	{
		BLE_RX.rx_flag ++;
		DMA_Cmd(DMA1_Channel3,DISABLE);
		BLE_RX.rx_len = USART_REC_LEN - DMA_GetCurrDataCounter(DMA1_Channel3);
		USART_ClearITPendingBit(USART3,USART_IT_IDLE);
		DMA_SetCurrDataCounter(DMA1_Channel3,USART_REC_LEN);
		DMA_Cmd(DMA1_Channel3,ENABLE);
		USART_ReceiveData(USART3);

		BLE_RX_Flag = 1;
		//BLE_TX(BLE_RX.rx_buf, BLE_RX.rx_len);
  } 
} 

