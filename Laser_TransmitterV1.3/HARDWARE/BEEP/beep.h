#ifndef  __BEEP_H_
#define  __BEEP_H_



#define  BEEP_PORT  	GPIOA
#define  BEEP_PIN			GPIO_Pin_12


#define  BEEP_OPEN		GPIO_SetBits(BEEP_PORT,BEEP_PIN)
#define  BEEP_CLOSE		GPIO_ResetBits(BEEP_PORT,BEEP_PIN)


void BEEP_Init(void);
void BEEP_OUTPUT(void);

#endif
