#ifndef  __MOTOR_MS3111S_
#define  __MOTOR_MS3111S_


#define  MS3111S_IN1_PORT		GPIOC
#define  MS3111S_IN1_PIN		GPIO_Pin_13

#define  MS3111S_IN2_PORT		GPIOC
#define  MS3111S_IN2_PIN		GPIO_Pin_14


#define  MS3111S_IN1_OUT		PCout(13)//PC13------IN1
#define  MS3111S_IN2_OUT		PCout(14)//PC14------IN2


void Motor_Ms3111s_Init(void);
void Machine_LockGun(void);
void Machine_UnlockGun(void);

#endif
