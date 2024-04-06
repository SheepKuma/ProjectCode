#ifndef _BLE_USART_H_
#define _BLE_USART_H_


extern volatile unsigned char BLE_RX_Flag;

void BLE_Usart3_Init(void);
void BLE_TX(unsigned char *tx_buff,unsigned char len);
void BLE_Proc(void);

#endif
