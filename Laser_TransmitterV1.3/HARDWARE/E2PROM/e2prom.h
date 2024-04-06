#ifndef __E2PROM_H_
#define __E2PROM_H_


#define I2C_SCL_PORT	GPIOA
#define I2C_SCL_PIN		GPIO_Pin_5

#define I2C_SDA_PORT	GPIOA
#define I2C_SDA_PIN		GPIO_Pin_4

#define EEPROM_PAGE_SIZE 32//64


void e2prom_Init(void);
void I2C_Read(unsigned short address,unsigned char *pBuffer, unsigned short len);
void I2C_PageWrite(unsigned short address,unsigned char *pDat, unsigned short num);


#endif
