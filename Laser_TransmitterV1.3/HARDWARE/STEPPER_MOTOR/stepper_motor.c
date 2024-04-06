#include "sys.h" 
#include "stepper_motor.h"
#include "usart.h"	
#include "delay.h"
#include "app.h"

static void M_I2C_Config(void);
//static void stepper_motor_config(void);
static void M_I2C_SDA_OUTPUT(unsigned char bVal);
static void M_I2C_SCL_OUTPUT(unsigned char bVal);
static void M_I2C_SDA_IO_Set(unsigned char IOMode);
static unsigned char M_I2C_SDA_INPUT(void);
static void M_I2C_delay(void);
static void M_I2C_Start(void);
static void M_I2C_Stop(void);
static void M_I2C_Ack(void);
static void M_I2C_NoAck(void);
static unsigned char M_I2C_WaitAck(void);
static void M_I2C_SendByte(unsigned char SendByte);
static unsigned char M_I2C_ReceiveByte(unsigned char acksta);


///0x40--32细分，正向；0x41--32细分，反向；0x00--256细分，正向；0x01--256细分，反向
///0xE0--1步进，正向；0xE1--1步进，反向；0xA0--4细分，正向；0xA1--4细分，反向
uint8_t s_data0=0xA0;;//0xA0;//0x00;//0x40;
uint8_t s_data1=0x16;
uint8_t s_data2=0x50;
uint8_t s_data3=0x4d;
uint8_t s_data4=0x01;



void Stepper_Motor_Init(void)
{
	M_I2C_Config();
	//stepper_motor_config();
}

//改动完
static void M_I2C_Config(void)
{
  GPIO_InitTypeDef  GPIO_InitStructure; 
  /* Configure I2C2 pins: PA5->SCL and PA4->SDA */
//  RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC | RCC_APB2Periph_GPIOA | RCC_APB2Periph_AFIO, ENABLE);
//	GPIO_PinRemapConfig(GPIO_Remap_SWJ_JTAGDisable, ENABLE);///关闭PA15脚的JTAG功能，PA15上电默认为JLINK的JTDI调试口,关闭此功能后才能作为普通IO口
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
	///SDA脚和SCL脚
  GPIO_InitStructure.GPIO_Pin =  M_SCL_PIN | M_SDA_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  
  GPIO_Init(M_SCL_PORT, &GPIO_InitStructure);
	GPIO_SetBits(M_SCL_PORT,M_SCL_PIN);
	GPIO_SetBits(M_SDA_PORT,M_SDA_PIN);
	
	M_I2C_SDA_OUTPUT(1);
  M_I2C_SCL_OUTPUT(1);
	
	///休眠脚，低电平有效
	GPIO_InitStructure.GPIO_Pin =  M_SLEEP_PIN;
  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;  
  GPIO_Init(M_SLEEP_PORT, &GPIO_InitStructure);
	GPIO_ResetBits(M_SLEEP_PORT,M_SLEEP_PIN);//根据DEMO先输出低电平
	//上电后先休眠
//	delay_us(2);//等待2us
//	GPIO_SetBits(M_SLEEP_PORT,M_SLEEP_PIN);//再输出高电平
	
	///异常输出脚，低电平有效
//	GPIO_InitStructure.GPIO_Pin =  M_NFAULT_PIN;
//  GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
//  GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;///上拉，异常时芯片向单片机输出低电平
//  GPIO_Init(M_NFAULT_PORT, &GPIO_InitStructure);
}

//电机关机
void Stepper_Motor_Close(void)
{
	GPIO_ResetBits(M_SLEEP_PORT,M_SLEEP_PIN);//低电平有效，给他关了
}
//电机开机
void Stepper_Motor_Open(void)
{
	GPIO_SetBits(M_SLEEP_PORT,M_SLEEP_PIN);//输出高电平，点击开机
}

//TODO已修改，带确认是否要改为13
static void M_I2C_delay(void)
{
   unsigned char i;
	///DEMO中是i<20，但DEMO的系统时钟是108MHz
	///移植到STM32的72MHz后，若需要达到同样计时数
	///是否应该把i最大值限制为72/108 * 20 = 13？
	for(i=0;i<20;i++);//for(i=0;i<20;i++);
}

//已更改
static void M_I2C_Start(void)
{
	M_I2C_SDA_OUTPUT(1);
	M_I2C_SCL_OUTPUT(1);
	delay_us(4);
	M_I2C_SDA_OUTPUT(0);
	delay_us(4);
	M_I2C_SCL_OUTPUT(0);
}
//已更改
static void M_I2C_Stop(void)
{
	M_I2C_SDA_OUTPUT(0);
	M_I2C_SCL_OUTPUT(0);
	delay_us(4);
	M_I2C_SCL_OUTPUT(1);
	delay_us(4);
	M_I2C_SDA_OUTPUT(1);
	delay_us(4);
}

//已更改
static void M_I2C_SendByte(unsigned char SendByte) 
{
  unsigned char i=0;
	unsigned char temp;
	temp = SendByte;
	
	M_I2C_SCL_OUTPUT(0);
	delay_us(1);
	for(i=0;i<8;i++)
	{
		if(temp&0x80)
		{
			M_I2C_SDA_OUTPUT(1);
		}else 
		{
			M_I2C_SDA_OUTPUT(0);
		}
		delay_us(2);
		M_I2C_SCL_OUTPUT(1);
		delay_us(2);
		M_I2C_SCL_OUTPUT(0);
		delay_us(2);
		temp<<=1;
	}
}

//已修改
static void M_I2C_Ack(void)
{	
	M_I2C_SCL_OUTPUT(0);
	M_I2C_SDA_OUTPUT(0);
	M_I2C_delay();
	M_I2C_SCL_OUTPUT(1);
	M_I2C_delay();
	M_I2C_SCL_OUTPUT(0);
}

//已修改
static void M_I2C_NoAck(void)
{	
	M_I2C_SCL_OUTPUT(0);
	M_I2C_SDA_OUTPUT(1);
	M_I2C_delay();
	M_I2C_SCL_OUTPUT(1);
	M_I2C_delay();
	M_I2C_SCL_OUTPUT(0);
}

//已修改
static unsigned char M_I2C_ReceiveByte(unsigned char acksta)  
{ 
	unsigned char i;
	unsigned char ReceiveByte=0;
	
	M_I2C_SDA_IO_Set(1);		 //SDA设置成输入
	M_I2C_delay();
	for(i=0; i<8; i++)
	{
		M_I2C_SCL_OUTPUT(0);
		delay_us(2);
		M_I2C_SCL_OUTPUT(1);
		ReceiveByte <<= 1;
		if(M_I2C_SDA_INPUT())
		{
			ReceiveByte|=0x01;
		}
		M_I2C_delay();
	}
	if(acksta == 1)
	{
		M_I2C_Ack();
	}
	else
	{
		M_I2C_NoAck();
	}
	M_I2C_SDA_IO_Set(0);		//SDA设置成输出
	M_I2C_delay();
	return ReceiveByte;
}

//已修改
static unsigned char M_I2C_WaitAck(void) 	
{
	unsigned char TimeCnt = 0;
	
	M_I2C_SDA_IO_Set(1);
	///此处DEMO中有个将SDA设置输出高电平的配置，逻辑上不通，故未加
	delay_us(1);	
	M_I2C_SCL_OUTPUT(1);
	if(M_I2C_SDA_INPUT())
	{
		TimeCnt ++;
		if(TimeCnt > 100)
		{
			M_I2C_Stop();
			return 1;
		}
	}
	M_I2C_SCL_OUTPUT(0);
	return 0;
}

//已修改,对应DEMO的write_Command()函数
void M_I2C_WriteByte(unsigned char addr, unsigned char map_addr,unsigned char dat)
{
//	unsigned char test = 1;///测试是否有应答
//	unsigned char test_buff[2] = {0x11,0x11};
	M_I2C_Start();
	
//	M_I2C_SendByte(0xA0);///写地址同样为A0，读的话是A1
	M_I2C_SendByte(addr);
	M_I2C_WaitAck();
//	test = M_I2C_WaitAck();
//	if(!test)
//	{
//		TypeC_Usart1_TX(test_buff, 2);
//	}
	
//	M_I2C_SendByte((address>>8)&0xFF);
//	M_I2C_WaitAck();
	///此芯片的寄存器MAP地址为1个字节，故只要发一次就行
	M_I2C_SendByte(map_addr);
	M_I2C_WaitAck();
	
	M_I2C_SendByte(dat);
	M_I2C_WaitAck();
	
	M_I2C_Stop();
}

//已修改，对应DEMO的read_Command
unsigned char M_I2C_ReadByte(unsigned char addr, unsigned char map_addr, unsigned char addr1)
{
	unsigned char dat = 0;
	M_I2C_Start();
//	M_I2C_SendByte(0xA0);
	M_I2C_SendByte(addr);
	M_I2C_WaitAck();

	M_I2C_SendByte(map_addr);
	M_I2C_WaitAck();
	M_I2C_Stop();

	M_I2C_Start();
	M_I2C_SendByte(addr1);
	M_I2C_WaitAck();
	
	dat = M_I2C_ReceiveByte(0);

	M_I2C_Stop();
	return dat;
}


void Steping(int mode)
{
		switch(mode)
	  {
		  case 1:	
			register_configuration(s_data0,s_data1,s_data2,s_data3,s_data4);  

			break;
	  	
		default:break;
	}
}

void register_configuration(uint8_t data0,uint8_t data1,uint8_t data2,uint8_t data3,uint8_t data4)
{ 
	M_I2C_WriteByte(0x20,0x01,data0);     //换向、细分数据
	M_I2C_WriteByte(0x20,0x03,data1);     //运行频率
	M_I2C_WriteByte(0x20,0x04,data2);     //步数
	M_I2C_WriteByte(0x20,0x06,data3);     //运行电流
	M_I2C_WriteByte(0x20,0x09,data4);     //程序载入、急停
}
///为了看的方便
//uint8_t s_data0=0x40;
//uint8_t s_data1=0x16;
//uint8_t s_data2=0x50;
//uint8_t s_data3=0x4d;
//uint8_t s_data4=0x01;





//已修改
//static void stepper_motor_config(void)
//{
//	M_I2C_WriteByte(0x20, 0x00, 0x21);  //寄存器使能通道打开
//	M_I2C_WriteByte(0x20, 0x07, 0x80);  //保持电流100%
//}
//检查ok
static void M_I2C_SDA_OUTPUT(unsigned char bVal)
{
 if(bVal)
	 {
		 GPIO_SetBits(M_SDA_PORT,M_SDA_PIN);
	 }else
	 {
		 GPIO_ResetBits(M_SDA_PORT,M_SDA_PIN);
	 }
}
//检查ok
static void M_I2C_SCL_OUTPUT(unsigned char bVal)
{
	 if(bVal)
	 {
		 GPIO_SetBits(M_SCL_PORT,M_SCL_PIN);
	 }else
	 {
		 GPIO_ResetBits(M_SCL_PORT,M_SCL_PIN);
	 }
}
//检查ok
static void M_I2C_SDA_IO_Set(unsigned char IOMode)
{
	if(IOMode == 0)					//输出
	{
		GPIO_InitTypeDef  GPIO_InitStructure; 
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
		GPIO_InitStructure.GPIO_Pin =   M_SDA_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;  
		GPIO_Init(M_SDA_PORT, &GPIO_InitStructure);
	}else if(IOMode == 1)			//输入	
	{
		GPIO_InitTypeDef  GPIO_InitStructure; 
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
		GPIO_InitStructure.GPIO_Pin =   M_SDA_PIN;
		GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
		GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU;  
		GPIO_Init(M_SDA_PORT, &GPIO_InitStructure);
	}
}
//检查ok
static unsigned char M_I2C_SDA_INPUT(void)
{
	return GPIO_ReadInputDataBit(M_SDA_PORT, M_SDA_PIN);		
}












//连续读多个字节
void M_I2C_Read(unsigned short address,unsigned char *pBuffer, unsigned short len)
{
	unsigned short length;
	length = len;
	M_I2C_Start();
//	M_I2C_SendByte(0xA0);
	M_I2C_SendByte(0x20);
	M_I2C_WaitAck();

//	M_I2C_SendByte((address>>8)&0xFF);
//	M_I2C_WaitAck();

	M_I2C_SendByte(address&0xFF);
	M_I2C_WaitAck();

	M_I2C_Start();
//	M_I2C_SendByte(0xA1);
	M_I2C_SendByte(0x21);
	M_I2C_WaitAck();
	
	//dat = I2C_ReceiveByte();
	 while(length)
    {
      *pBuffer = M_I2C_ReceiveByte(1);
      if(length == 1)M_I2C_NoAck();
      else M_I2C_Ack(); 
      pBuffer++;
      length--;
    }
	 
	M_I2C_Stop();
	 
}

