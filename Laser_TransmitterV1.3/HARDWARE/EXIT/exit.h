#ifndef _EXIT_H_
#define _EXIT_H_


#define HALL_PROT  GPIOB
#define HALL_PIN   GPIO_Pin_9

#define HALL_KEY  	GPIO_ReadInputDataBit(HALL_PROT,HALL_PIN)

extern volatile unsigned char Timed_ShootFlag;

void Exti_Init(void);
void Reset_ShootFlag(void);


#endif
