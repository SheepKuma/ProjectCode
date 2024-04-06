#ifndef  __STEPPER_MOTOR_H_
#define  __STEPPER_MOTOR_H_
#include "sys.h" 

//电机驱动I2C的SCL脚
#define  M_SCL_PORT			GPIOC
#define  M_SCL_PIN			GPIO_Pin_9
//电机驱动I2C的SDA脚
#define  M_SDA_PORT			GPIOC
#define  M_SDA_PIN			GPIO_Pin_8
//电机驱动的MSLEEP脚
#define  M_SLEEP_PORT		GPIOC
#define  M_SLEEP_PIN		GPIO_Pin_10
//电机驱动的nFault脚
#define  M_NFAULT_PORT	GPIOA
#define  M_NFAULT_PIN		GPIO_Pin_15



void Stepper_Motor_Init(void);
void M_I2C_WriteByte(unsigned char addr, unsigned char map_addr,unsigned char dat);
unsigned char M_I2C_ReadByte(unsigned char addr, unsigned char map_addr, unsigned char addr1);
void M_I2C_Read(unsigned short address,unsigned char *pBuffer, unsigned short len);
void register_configuration(uint8_t data0,uint8_t data1,uint8_t data2,uint8_t data3,uint8_t data4);
void Steping(int mode);
void Stepper_Motor_Close(void);
void Stepper_Motor_Open(void);

#endif
