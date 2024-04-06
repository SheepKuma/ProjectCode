#ifndef _ADC_H_
#define _ADC_H_



void  Adc_Init(void);
void ADC_Cycle(void);
void ADC_Proc(void);
unsigned char Query_BatState(void);
unsigned char Get_Battery(void);
void ADC_PowerON(void);//上电后检查电量

#endif


