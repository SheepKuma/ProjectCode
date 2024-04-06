#include "sys.h"
#include "usart.h"	  
#include "app.h"
#include <string.h>

//////////////////////////////////////////////Printf重定向////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
void _sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 

/*使用microLib的方法*/
 /* 
int fputc(int ch, FILE *f)
{
	USART_SendData(USART1, (uint8_t) ch);

	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}	
   
    return ch;
}
int GetKey (void)  { 

    while (!(USART1->SR & USART_FLAG_RXNE));

    return ((int)(USART1->DR & 0x1FF));
}
*/
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////USART1//////////////////////////////////////

usart_rx_data TypeC_RX={0};
unsigned char TypeC_DebugBuff[USART_REC_LEN]={0};	
volatile unsigned char TypeC_RX_Flag = 0;

static void Usart1_Config(u32 bound);
static void USART1_DMA_RX_config(void);
static void Usart1_SendByte(unsigned char  Dat);

void TypeC_Usart1_Init(void)
{
	Usart1_Config(115200);
	USART1_DMA_RX_config();
	
	TypeC_RX_Flag = 0;
}

static void Usart1_Config(u32 bound)
{ 
	GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure; 
	NVIC_InitTypeDef NVIC_InitStructure;
 
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOA,ENABLE);
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_USART1,ENABLE);
	 
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;	         
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;  
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOA, &GPIO_InitStructure);		   

	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;	        
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;  
	GPIO_Init(GPIOA, &GPIO_InitStructure);

	USART_InitStructure.USART_BaudRate = bound;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;
	USART_InitStructure.USART_StopBits = USART_StopBits_1;
	USART_InitStructure.USART_Parity = USART_Parity_No;
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;

	USART_Init(USART1, &USART_InitStructure); 
	USART_ITConfig(USART1, USART_IT_IDLE, ENABLE);
	USART_DMACmd(USART1,USART_DMAReq_Rx,ENABLE);
	USART_Cmd(USART1, ENABLE);
	
	NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
}

static void USART1_DMA_RX_config(void)
{
	DMA_InitTypeDef DMA_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);	//使能DMA传输

	DMA_InitStructure.DMA_PeripheralBaseAddr = (uint32_t)(&USART1->DR);  //DMA外设基地址
	DMA_InitStructure.DMA_MemoryBaseAddr = (uint32_t)TypeC_RX.rx_buf;  //DMA内存基地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralSRC;  //数据传输方向
	DMA_InitStructure.DMA_BufferSize = USART_REC_LEN;  //DMA通道的DMA缓存的大小
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;  //外设地址寄存器不变
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;  //内存地址寄存器递增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;  //数据宽度为8位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte; //数据宽度为8位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;  //工作在正常模式
	DMA_InitStructure.DMA_Priority = DMA_Priority_Medium; //DMA通道 x拥有中优先级 
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;  //DMA通道x没有设置为内存到内存传输
	DMA_Init(DMA1_Channel5, &DMA_InitStructure);  //根据DMA_InitStruct中指定的参数初始化DMA的通道USART1_Tx_DMA_Channe5所标识的寄存器

	DMA_Cmd(DMA1_Channel5,ENABLE);
	DMA_ITConfig(DMA1_Channel5,DMA_IT_TC,ENABLE);
}


static void Usart1_SendByte(unsigned char  Dat)
{
	USART_SendData(USART1, Dat);
	while(USART_GetFlagStatus (USART1 ,USART_FLAG_TXE )!=SET );
}

void TypeC_Usart1_TX(u8 *tx_buff,u8 len)
{
	int i = 0;
	for(;i<len; i++)
	{
		Usart1_SendByte(tx_buff[i]);//向串口1发送数据
	}
	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
}

void TypeC_Proc(void)
{
	//TypeC_Usart1_TX(TypeC_RX.rx_buf, TypeC_RX.rx_len);
	
	unsigned char len = 0;
	
	len = TypeC_RX.rx_len;
	memcpy(TypeC_DebugBuff,TypeC_RX.rx_buf,len);
	if((TypeC_DebugBuff[0] == BLE_HEADER) && (TypeC_DebugBuff[len-1] == BLE_TAIL)) 
	{
			//TODO:加入Type-C串口的数据接收处理
		//BLE_DataHandle(TypeC_DebugBuff, len);
	}
	TypeC_RX_Flag = 0;
}

void USART1_IRQHandler(void)
{
	if(USART_GetITStatus(USART1,USART_IT_IDLE) != RESET)                   
	{
		DMA_Cmd(DMA1_Channel5,DISABLE);
		
		TypeC_RX.rx_len = USART_REC_LEN - DMA_GetCurrDataCounter(DMA1_Channel5);
		USART_ClearITPendingBit(USART1,USART_IT_IDLE);
		DMA_SetCurrDataCounter(DMA1_Channel5,USART_REC_LEN);
		DMA_Cmd(DMA1_Channel5,ENABLE);
		USART_ReceiveData(USART1);
		
		TypeC_RX_Flag = 1;
		//usart_debugProc();
	}
}

