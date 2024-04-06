#ifndef __USART_H
#define __USART_H
#include "stdio.h"	
#include "sys.h" 
//////////////////////////////////////////////////////////////////////////////////	 


#define USART_REC_LEN  			150  	//定义最大接收字节数 200

typedef struct{
	u8 rx_flag;
	u8 rx_len;
	u8 rx_buf[USART_REC_LEN];
}usart_rx_data;

extern volatile unsigned char TypeC_RX_Flag;
	  	
void TypeC_Usart1_Init(void);
void TypeC_Usart1_TX(u8 *tx_buff,u8 len);
void TypeC_Proc(void);


#endif


