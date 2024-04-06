#include "sys.h" 
#include "ir.h"
#include "delay.h"
#include "usart.h"	  
#include "app.h"
#include "e2prom.h"
#include <string.h>
#include "mytimer.h"
#include "ble_usart.h"
#include "motor_ms3111s.h"
#include "adc.h"
#include "mytimer2.h"


/*******************************************************************************
使用说明：
一、外部调用函数为:
1.初始化函数Ir_Init();
2.激光发射函数Ir_Shot()，每调用一次发射一次激光；
3.串口更改激光参数函数GunID_ChangeI(unsigned char gunid_type, unsigned char cntkey, unsigned char ir_freq, unsigned char *gunid_buff)，参数见该函数注释

二、需注意更改的地方:
1.e2prom擦写操作;
2.若需要添加每次发射激光后上报蓝牙，则在Ir_Shot()函数中添加串口发送相关代码。
*******************************************************************************/


	

static void Ir_Config(unsigned short arr, unsigned short psc);
static void Ir_DMAConfig(DMA_Channel_TypeDef * DMAx_Channely, unsigned long PerAddr, unsigned long MemAddr, unsigned long BuffSize);
static void GunIR_InfoInit(void);
static void GunID_To_IRDMABuff(unsigned char gunid_type, unsigned char cntkey, unsigned char ir_freq, unsigned char *gunid_buff);
static void IR_InfoSet(void);
static void DMA_Buff_CntUpdate(unsigned char cnt);
static void Shot_BLEReport(unsigned short shot_cnt, unsigned short bullet_remian);


//直接初始化为带计数位的buff，若是不带计数位，则改变DMA传输的长度，以及buff中帧尾的位置即可
unsigned short IrDMA_1ByteBuff[GUNID_1BYTE_CNT_LEN] = {0};//一字节枪号对应的DMA数组,不带计数位
unsigned short IrDMA_3ByteBuff[GUNID_3BYTE_CNT_LEN] = {0};//三字节枪号对应的DMA数组,不带计数位

//默认16进制的枪号数组(预留一个计数位)，当蓝牙MAC地址为C1 21 00 00 00 00时，就用这个枪号
unsigned char GunID_Buff[5]={0xff,0x00,0x21,0,0};
//unsigned char GunID_Buff[4]={0x01,0x00,0x80,0};
//unsigned char GunID_Buff[4]={0xff,0xff,0xEF,0};


unsigned char frame_num = 0;//一次激光发射的PWM帧数
unsigned char frame_cnt = 0;//帧数计数，每发一帧就+1
IR_INFO_STU IR_Info = {0};//激光参数的结构体变量
GUN_INFO_STU Gun_Info = {0};//枪械参数的结构体变量
unsigned char DMA_BuffLen = 0;//用于发送完一帧PWM后，重新设置DMA通道的DMA缓存的大小
unsigned char Cnt_Flag = 0;//计数的标志位，为1时表示激光中需要有计数位，为0时表示不需要加计数位
unsigned char IRSendNum = 0;//激光发射的计数
unsigned char GunLock_Flag = 0;//锁枪标志位，当子弹为0时置起，等待扳机松开后再锁枪，避免没松扳机就锁枪，导致锁枪后还能扣动扳机


/*******************************************************************************
* Function Name  : void Ir_Init(void)
* Description    : 用于初始化定时器、PWM和DMA的相关配置，在main函数中调用
* Input          : None
* Output         : None
* Return         : None
* Attention		 	 : None
*******************************************************************************/
void Ir_Init(void)
{
	//////TEST
//	memcpy(IR_Info.ID_Buff, GunID_Buff, 3);
//	IR_Info.Cnt_Key = Cnt_Key_On;
//	IR_Info.ID_Type = GUNID_3Byte;
//	IR_Info.IR_Freq = Freq_48K;
//	frame_num = IR_Info.Send_Frame = 3;
//	I2C_PageWrite(0, (unsigned char *)&IR_Info, sizeof(IR_Info));
	//STMFLASH_Write(IRINFO_SAVE_ADDR, (u16*)&IR_Info, sizeof(IR_Info));
	//////TEST END

	IR_InfoSet();//从flash/e2prom中读出激光相关参数
	//根据激光频率初始化定时器的配置
	switch(IR_Info.IR_Freq)
	{
		case Freq_48K:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//修改频率
			break;
		case Freq_500K:
			Ir_Config(CYCLE_TICK_500K-1,3-1);//修改频率
			break;
		case Freq_200K:
			Ir_Config(CYCLE_TICK_200K-1,3-1);//修改频率
			break;
		default:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//修改频率
			break;
	}
	//根据枪号类型初始化DMA配置
	if(IR_Info.ID_Type == GunID_1Byte)
	{
		if(Cnt_Flag)//根据计数标志位决定DMA buff的长度
		{
			//IRSendNum = 1;//从1开始计数
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_1ByteBuff, GUNID_1BYTE_CNT_LEN);
			DMA_BuffLen = GUNID_1BYTE_CNT_LEN;
		}
		else
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_1ByteBuff, GUNID_1BYTE_LEN);
			DMA_BuffLen = GUNID_1BYTE_LEN;
		}
	}
	else
	{
		if(Cnt_Flag)//根据计数标志位决定DMA buff的长度
		{
			//IRSendNum = 1;//从1开始计数
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_3ByteBuff, GUNID_3BYTE_CNT_LEN);
			DMA_BuffLen = GUNID_3BYTE_CNT_LEN;
		}
		else
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_3ByteBuff, GUNID_3BYTE_LEN);
			DMA_BuffLen = GUNID_3BYTE_LEN;
		}
	}

	GunIR_InfoInit();//初始化枪械与激光的相关参数
	Mode_Change_GunID();
}
///两种方案(注意：更改频率时要记得更改GunID_To_IRDMABuff()函数填充高电平的BUFF值)：
///方案一：24MHz
///48K： Ir_Config(500-1,3-1),高电平TICK值为72，此时为很准的20.83US周期，3US高电平
///200K: Ir_Config(120-1,3-1),高电平TICK值为60，此时周期为5US，2.5US高电平
///500K：Ir_Config(48-1,3-1),高电平TICK值为24， 此时周期为2US， 2US高电平

///方案二：8MHz
///48K: Ir_Config(167-1,9-1),高电平TICK值为24，此时周期为20.88US，3US高电平
///200K：Ir_Config(40-1,9-1),高电平TICK值为20，此时周期为5US，2.5US高电平
///500K: Ir_Config(16-1,9-1),高电平TICK值为8，此时周期为2US，1US高电平

///另外，由于PWM设置中TIM_OCInitStructure.TIM_Pulse为0，故从外部中断触发到发射激光，之间会有延时，且与频率相关


/*******************************************************************************
* Function Name  : static void Ir_Config(unsigned short arr, unsigned short psc)
* Description    : 用于初始化定时器、PWM的相关配置
* Input          : unsigned short arr--周期
									 unsigned short psc--定时器输入时钟的预分频值
* Output         : None
* Return         : None
* Attention		 	 : None
*******************************************************************************/
static void Ir_Config(unsigned short arr, unsigned short psc)
{
	GPIO_InitTypeDef GPIO_InitStructure;
	TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	TIM_OCInitTypeDef  TIM_OCInitStructure;
	
	RCC_APB2PeriphClockCmd( RCC_APB2Periph_GPIOB, ENABLE); 	
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);	
	
   //PB8配置
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
 
   //初始化TIM4
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //根据TIM_TimeBaseInitStruct中指定的参数初始化TIMx的时间基数单位
	
	//初始化TIM4 Channel3 PWM模式	 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //选择定时器模式:TIM脉冲宽度调制模式1
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //比较输出使能
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //输出极性:TIM输出比较极性高
	TIM_OCInitStructure.TIM_Pulse = 0;//0;//IrDMA_Buff[0]
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);  //根据T指定的参数初始化外设TIM4 OC3
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);  //使能TIM4在CCR3上的预装载寄存器
	//TIM_ARRPreloadConfig(TIM4, ENABLE);
	//TIM_GenerateEvent(TIM4, TIM_EventSource_Update);
	TIM_DMACmd(TIM4, TIM_DMA_Update, ENABLE);
	//TIM_Cmd(TIM4, ENABLE);  //使能TIM4
	//TIM_Cmd(TIM4, DISABLE);
	
}

/*******************************************************************************
* Function Name  : static void Ir_DMAConfig(DMA_Channel_TypeDef * DMAx_Channely, unsigned long PerAddr, unsigned long MemAddr, unsigned long BuffSize)
* Description    : 用于配置DMA相关参数、中断配置
* Input          : DMA_Channel_TypeDef * DMAx_Channely--所用到的DMA通道
									 unsigned long PerAddr--DMA数据传输的目标地址，即定时器的CCR寄存器
									 unsigned long MemAddr--DMA数据缓存地址
									 unsigned long BuffSize--DMA数据缓存区大小
* Output         : None
* Return         : None
* Attention		 	 : None
*******************************************************************************/
static void Ir_DMAConfig(DMA_Channel_TypeDef * DMAx_Channely, unsigned long PerAddr, unsigned long MemAddr, unsigned long BuffSize)
{
	DMA_InitTypeDef DMA_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);//启动DMA时钟
 
	DMA_DeInit(DMAx_Channely);
	while ( 0 != DMA_GetCurrDataCounter(DMAx_Channely)){}//等待DMA可配置
	
	//配置负责PB8 PWM发送的DMA    
	DMA_InitStructure.DMA_PeripheralBaseAddr = PerAddr;			//数据传输目标地址
	DMA_InitStructure.DMA_MemoryBaseAddr = MemAddr;			//数据缓存地址
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;									//外设作为数据传输的目的地
	DMA_InitStructure.DMA_BufferSize = BuffSize;						//发送Buff数据大小
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	 //设置外设地址是否递增
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;				 //设置内存地址是否递增
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	//外设数据宽度为8位
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;		 	//内存数据宽度为8位
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;		  	//普通缓存模式
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;	   //最高优先级
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;	  		//禁止DMA2个内存相互访问
	DMA_Init(DMAx_Channely, &DMA_InitStructure);		  	//初始化DMA
}

//初始化枪械与激光的相关参数
static void GunIR_InfoInit(void)
{
	IRSendNum = 1;//从1开始计数
	frame_cnt = 0;//帧计数初始化
	Gun_Info.Gun_Lock = Unlocked_Gun;//上电不锁枪，先进入测试设备模式，配30发子弹
	Gun_Info.Gun_Mode = Dev_TestMode;//上电不锁枪，先进入测试设备模式，配30发子弹
	Gun_Info.Bullet_Num = 30;//上电不锁枪，先进入测试设备模式，配30发子弹
	Gun_Info.Shoot_Cnt = 0;//从1开始计数
	GunLock_Flag = 0;
}



/*******************************************************************************
* Function Name  : void Ir_Shot(void)
* Description    : 激光发射函数，供外部调用，执行一次发送一次激光
* Input          : None
* Output         : None
* Return         : None
* Attention		 	 : None
*******************************************************************************/
void Ir_Shot(void)
{
	if(Gun_Info.Gun_Lock == Unlocked_Gun)//没锁枪状态下才可以发射激光
	{
		do
		{
			TIM4->CR1 |= TIM_CR1_CEN;//打开定时器
			DMA1_Channel7->CCR &= (uint16_t)(~DMA_CCR1_EN);//关闭DMA通道
			DMA1_Channel7->CNDTR = DMA_BuffLen;//设置DMA通道的DMA缓存的大小
			DMA1_Channel7->CCR |= DMA_CCR1_EN;//打开DMA通道

			while (DMA_GetCurrDataCounter(DMA1_Channel7));
			frame_cnt++;
		}while(frame_cnt<frame_num);
		TIM_Cmd(TIM4, DISABLE);
		DMA1_Channel7->CCR &= (uint16_t)(~DMA_CCR1_EN);//关闭DMA通道
		frame_cnt=0;
		
		//if((Gun_Info.Gun_Mode == Dev_TestMode) || (Gun_Info.Gun_Mode == Normal_ShootMode))//当处于普通射击模式和测试设备模式下，处理子弹数与击发数，并上报蓝牙
		if((Gun_Info.Gun_Mode != Unlimited_BulletMode) && (Gun_Info.Gun_Mode != Calibrate_Mode))//只要不是无限子弹模式和校准模式，就处理子弹数
		{
			//击发数增加
			if(Gun_Info.Shoot_Cnt == 65535)
			{
				Gun_Info.Shoot_Cnt = 0;
			}
			else
			{
				Gun_Info.Shoot_Cnt++;
			}
			//子弹数减少
			Gun_Info.Bullet_Num--;
			if(Gun_Info.Bullet_Num == 0)
			{
				Gun_Info.Gun_Lock = Locked_Gun;//锁枪
				GunLock_Flag = 1;
				//Machine_LockGun();//锁枪
				if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//更新菜单页面
				{
					Set_ClrScreenFlag();
				}
			}
			Shot_BLEReport(Gun_Info.Shoot_Cnt, Gun_Info.Bullet_Num);//上报蓝牙
		}
		//当处于无限子弹模式下，就只处理击发数，不处理子弹数，然后上报蓝牙
		else if(Gun_Info.Gun_Mode == Unlimited_BulletMode)
		{
			//击发数增加
			if(Gun_Info.Shoot_Cnt == 65535)
			{
				Gun_Info.Shoot_Cnt = 0;
			}
			else
			{
				Gun_Info.Shoot_Cnt++;
			}
			Shot_BLEReport(0xffff, 0xffff);//上报蓝牙都是0xffff是根据协议来的，也可以把Gun_Info.Shoot_Cnt传进去
		}
		else if(Gun_Info.Gun_Mode == Calibrate_Mode)//当处于校准模式下，击发数和子弹数都不处理，只上报蓝牙
		{
			Shot_BLEReport(0xffff, 0xffff);
		}
		
		//波形中计数位增加
		if(IRSendNum == 0xFF)
		{
			IRSendNum = 1;//从1开始计数
		}
		else
		{
			IRSendNum++;//激光发射计数，每发射一次激光就+1
		}
		if(Cnt_Flag)//若使能了计数标志位，则每次发射完激光，都更新下次待发送的DMA buff
		{
			DMA_Buff_CntUpdate(IRSendNum);//根据最新的计数值，更新下次待发送的DMA buff
		}
	}
		
}


/*******************************************************************************
* Function Name  : static void IR_InfoSet(void)
* Description    : 用于设置激光相关参数(激光频率、枪号位数、PWM帧数)，在Ir_Init()初始化时调用
* Input          : None
* Output         : None
* Return         : None
* Attention		 	 : 当发现flash中没有保存相关参数时，程序默认设置成48K频率、3字节枪号、3帧PWM、枪号68-02-19
*******************************************************************************/
static void IR_InfoSet(void)
{
	unsigned char ff_buff[3] = {0xff,0xff,0xff};
	///TODO:加入从E2PROM读写
	I2C_Read(0, (unsigned char *)&IR_Info, sizeof(IR_Info));
	//STMFLASH_ReadForByte(IRINFO_SAVE_ADDR, (u8*)&IR_Info, sizeof(IR_Info));
	if((IR_Info.ID_Type != 0xff) && (IR_Info.IR_Freq != 0xff) && (IR_Info.Send_Frame != 0xff) && (IR_Info.Cnt_Key != 0xff) && (memcmp(IR_Info.ID_Buff, ff_buff, 3) != 0))
	{
		memset(GunID_Buff, 0, 5);///先清空一下GunID_Buff，防止之前的计数位影响到校验和的计算
		memcpy(GunID_Buff,IR_Info.ID_Buff,3);
		if(GunID_Buff[0] == 0)//当ID为0时，就把ID赋给ff，避免衣服不解码
		{
			GunID_Buff[0] = 0xff;
		}
		Cnt_Flag = IR_Info.Cnt_Key;
		GunID_Buff[4] = Check_Sum(GunID_Buff,3);//算出校验和
		GunID_To_IRDMABuff(IR_Info.ID_Type, Cnt_Flag, IR_Info.IR_Freq, GunID_Buff);
		frame_num = IR_Info.Send_Frame;
	}
	else
	{  ///默认48K频率，3字节枪号带计数位，激光帧数为3帧
		memcpy(IR_Info.ID_Buff, GunID_Buff, 3);
		IR_Info.ID_Type = GUNID_3Byte;
		Cnt_Flag = IR_Info.Cnt_Key = Cnt_Key_On;
		IR_Info.IR_Freq = Freq_48K;
		frame_num = IR_Info.Send_Frame = 3;

		I2C_PageWrite(0, (unsigned char *)&IR_Info, sizeof(IR_Info));
		//STMFLASH_Write(IRINFO_SAVE_ADDR, (u16*)&IR_Info, sizeof(IR_Info));
		
		GunID_Buff[4] = Check_Sum(GunID_Buff,3);
		GunID_To_IRDMABuff(IR_Info.ID_Type, Cnt_Flag, IR_Info.IR_Freq, GunID_Buff);
	}
}

/*******************************************************************************
* Function Name  : void GunID_Change(unsigned char gunid_type, unsigned char cntkey, unsigned char ir_freq, unsigned char sendframe, unsigned char *gunid_buff)
* Description    : 用于改变激光相关参数(激光频率、枪号位数、PWM帧数),当串口收到相应指令时调用此函数更改激光参数
* Input          : unsigned char gunid_type--枪号位数(1个字节/3个字节)
									 unsigned char cntkey--计数开关，为1时激光带计数位，为0时不带计数位
									 unsigned char ir_freq--激光频率(48K/200K/500K)
									 unsigned char sendframe--PWM帧数(1-255)
									 unsigned char *gunid_buff--具体的枪号
* Output         : None
* Return         : None
* Attention		 	 : None
*******************************************************************************/
void GunID_Change(unsigned char gunid_type, unsigned char cntkey, unsigned char ir_freq, unsigned char sendframe, unsigned char *gunid_buff)
{
	unsigned char type = gunid_type, freq = ir_freq; 
	unsigned char change_mac_flag = 0;//更改MAC的标志位，当收到的枪号不全为0时，就通知蓝牙模块更改MAC地址
	unsigned char zero_buff[3] = {0x00,0x00,0x00};
	
	IR_Info.Cnt_Key = Cnt_Flag = cntkey;
	IR_Info.Send_Frame = frame_num = sendframe;
	IR_Info.ID_Type = type;
	IR_Info.IR_Freq = freq;

	switch(IR_Info.IR_Freq)
	{
		case Freq_48K:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//修改频率
			break;
		case Freq_500K:
			Ir_Config(CYCLE_TICK_500K-1,3-1);//修改频率
			break;
		case Freq_200K:
			Ir_Config(CYCLE_TICK_200K-1,3-1);//修改频率
			break;
		default:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//修改频率
			break;
	}
	
	if(type == GunID_1Byte)
	{
		if(memcmp(gunid_buff, zero_buff, 3))//如果0x04设置激光参数指令发过来的枪号全是0，那么就不更改枪号
		{
			memset(GunID_Buff, 0, 5);///先清空一下GunID_Buff，防止之前的计数位影响到校验和的计算//虽然1字节没有校验，但更改枪号前还是清一下
			GunID_Buff[0] = gunid_buff[0];
			change_mac_flag = 1;//标志需要更改MAC地址
		}
		//由于频率可能更改了，所以就算0x04设置激光参数指令发过来的枪号全是0，也会重新装载DMA数组
		GunID_To_IRDMABuff(gunid_type, Cnt_Flag, freq, GunID_Buff);
		if(Cnt_Flag)
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_1ByteBuff, GUNID_1BYTE_CNT_LEN);
			DMA_BuffLen = GUNID_1BYTE_CNT_LEN;
		}
		else
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_1ByteBuff, GUNID_1BYTE_LEN);
			DMA_BuffLen = GUNID_1BYTE_LEN;
		}
	}
	else
	{
		if(memcmp(gunid_buff, zero_buff, 3))//如果0x04设置激光参数指令发过来的枪号全是0，那么就不更改枪号
		{
			memset(GunID_Buff, 0, 5);///先清空一下GunID_Buff，防止之前的计数位影响到校验和的计算
			memcpy(GunID_Buff,gunid_buff,3);
			GunID_Buff[4] = Check_Sum(GunID_Buff,3);
			change_mac_flag = 1;//标志需要更改MAC地址
		}
		//由于频率可能更改了，所以就算0x04设置激光参数指令发过来的枪号全是0，也会重新装载DMA数组
		GunID_To_IRDMABuff(gunid_type, Cnt_Flag, freq, GunID_Buff);
		if(Cnt_Flag)
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_3ByteBuff, GUNID_3BYTE_CNT_LEN);
			DMA_BuffLen = GUNID_3BYTE_CNT_LEN;
		}
		else
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_3ByteBuff, GUNID_3BYTE_LEN);
			DMA_BuffLen = GUNID_3BYTE_LEN;
		}
		
	}
	
	memcpy(IR_Info.ID_Buff,GunID_Buff,3);
	I2C_PageWrite(0, (unsigned char *)&IR_Info, sizeof(IR_Info));
	//STMFLASH_Write(IRINFO_SAVE_ADDR, (u16*)&IR_Info, sizeof(IR_Info));
	
	//如果更改MAC地址的标志位被置起,那就说明要改MAC地址了
	if(change_mac_flag)
	{
		BLE_Mac_Change(GunID_Buff);
	}

}

//只更改枪号，不更改其他激光参数
//用于每次BLE主机(显控)更改BLE从机的MAC后，从机主动通过串口通知STM32改变枪号，此时就不需要改变其他参数，只改枪号就行
void Gun_Change_OnlyID(unsigned char *gunid_buff)
{
	unsigned char zero_buff[3] = {0x00,0x00,0x00};

	switch(IR_Info.IR_Freq)
	{
		case Freq_48K:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//修改频率
			break;
		case Freq_500K:
			Ir_Config(CYCLE_TICK_500K-1,3-1);//修改频率
			break;
		case Freq_200K:
			Ir_Config(CYCLE_TICK_200K-1,3-1);//修改频率
			break;
		default:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//修改频率
			break;
	}
	
	if(IR_Info.ID_Type == GunID_1Byte)
	{
		if(memcmp(gunid_buff, zero_buff, 3) != 0)//如果0x04设置激光参数指令发过来的枪号全是0，那么就不更改枪号
		{
			memset(GunID_Buff, 0, 5);///先清空一下GunID_Buff，防止之前的计数位影响到校验和的计算//虽然1字节没有校验，但更改枪号前还是清一下
			GunID_Buff[0] = gunid_buff[0];
		}
		//由于频率可能更改了，所以就算0x04设置激光参数指令发过来的枪号全是0，也会重新装载DMA数组
		GunID_To_IRDMABuff(IR_Info.ID_Type, Cnt_Flag, IR_Info.IR_Freq, GunID_Buff);
		if(Cnt_Flag)
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_1ByteBuff, GUNID_1BYTE_CNT_LEN);
			DMA_BuffLen = GUNID_1BYTE_CNT_LEN;
		}
		else
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_1ByteBuff, GUNID_1BYTE_LEN);
			DMA_BuffLen = GUNID_1BYTE_LEN;
		}
	}
	else
	{
		if(memcmp(gunid_buff, zero_buff, 3) != 0)//如果0x04设置激光参数指令发过来的枪号全是0，那么就不更改枪号
		{
			memset(GunID_Buff, 0, 5);///先清空一下GunID_Buff，防止之前的计数位影响到校验和的计算
			memcpy(GunID_Buff,gunid_buff,3);
			GunID_Buff[4] = Check_Sum(GunID_Buff,3);
		}
		//由于频率可能更改了，所以就算0x04设置激光参数指令发过来的枪号全是0，也会重新装载DMA数组
		GunID_To_IRDMABuff(IR_Info.ID_Type, Cnt_Flag, IR_Info.IR_Freq, GunID_Buff);
		if(Cnt_Flag)
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_3ByteBuff, GUNID_3BYTE_CNT_LEN);
			DMA_BuffLen = GUNID_3BYTE_CNT_LEN;
		}
		else
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_3ByteBuff, GUNID_3BYTE_LEN);
			DMA_BuffLen = GUNID_3BYTE_LEN;
		}
		
	}
	
	memcpy(IR_Info.ID_Buff,GunID_Buff,3);
	I2C_PageWrite(0, (unsigned char *)&IR_Info, sizeof(IR_Info));
	//STMFLASH_Write(IRINFO_SAVE_ADDR, (u16*)&IR_Info, sizeof(IR_Info));

}


//用来在设备测试模式与其他正式比赛模式间切换激光波形中的枪号
//设备测试模式下枪号的ID为ff，其他模式下枪号为正常的ID号
void Mode_Change_GunID(void)
{
	/*switch(IR_Info.IR_Freq)
	{
		case Freq_48K:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//修改频率
			break;
		case Freq_500K:
			Ir_Config(CYCLE_TICK_500K-1,3-1);//修改频率
			break;
		case Freq_200K:
			Ir_Config(CYCLE_TICK_200K-1,3-1);//修改频率
			break;
		default:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//修改频率
			break;
	}*/
	
	if(IR_Info.ID_Type == GunID_1Byte)
	{
		memset(GunID_Buff, 0, 5);///先清空一下GunID_Buff，防止之前的计数位影响到校验和的计算//虽然1字节没有校验，但更改枪号前还是清一下
		memcpy(GunID_Buff,IR_Info.ID_Buff,3);
		if(Gun_Info.Gun_Mode == Dev_TestMode)
		{
			GunID_Buff[0] = 0xff;
		}
		//重新装载DMA数组
		GunID_To_IRDMABuff(IR_Info.ID_Type, Cnt_Flag, IR_Info.IR_Freq, GunID_Buff);
		/*if(Cnt_Flag)
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_1ByteBuff, GUNID_1BYTE_CNT_LEN);
			DMA_BuffLen = GUNID_1BYTE_CNT_LEN;
		}
		else
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_1ByteBuff, GUNID_1BYTE_LEN);
			DMA_BuffLen = GUNID_1BYTE_LEN;
		}*/
	}
	else
	{
		memset(GunID_Buff, 0, 5);///先清空一下GunID_Buff，防止之前的计数位影响到校验和的计算
		memcpy(GunID_Buff,IR_Info.ID_Buff,3);
		if(Gun_Info.Gun_Mode == Dev_TestMode)
		{
			GunID_Buff[0] = 0xff;
		}
		GunID_Buff[4] = Check_Sum(GunID_Buff,3);

		//重新装载DMA数组
		GunID_To_IRDMABuff(IR_Info.ID_Type, Cnt_Flag, IR_Info.IR_Freq, GunID_Buff);
		/*if(Cnt_Flag)
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_3ByteBuff, GUNID_3BYTE_CNT_LEN);
			DMA_BuffLen = GUNID_3BYTE_CNT_LEN;
		}
		else
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_3ByteBuff, GUNID_3BYTE_LEN);
			DMA_BuffLen = GUNID_3BYTE_LEN;
		}*/
		
	}
}

/*******************************************************************************
* Function Name  : static void GunID_To_IRDMABuff(unsigned char gunid_type, unsigned char cntkey, unsigned char ir_freq, unsigned char *gunid_buff)
* Description    : 根据激光的参数，将具体枪号转化成待传输的DMA数组，此函数在GunID_Change()中调用
* Input          : unsigned char gunid_type--枪号位数(1个字节/3个字节)
									 unsigned char cntkey--计数开关，决定激光中是否带计数位
									 unsigned char ir_freq--激光频率(48K/200K/500K)
									 unsigned char *gunid_buff--具体的枪号
* Output         : None
* Return         : None
* Attention		 	 : None
*******************************************************************************/
static void GunID_To_IRDMABuff(unsigned char gunid_type, unsigned char cntkey, unsigned char ir_freq, unsigned char *gunid_buff)
{
	unsigned char highbit_tick = 0, idtype = gunid_type, gunid= 0;
	unsigned char i = 0, j = 0, bit = 0, num = 0;
	
	switch(ir_freq)
	{
		case Freq_48K:
			highbit_tick = HIGHBIT_TICK_48K;
		break;
		
		case Freq_500K:
			highbit_tick = HIGHBIT_TICK_500K;
		break;
		
		case Freq_200K:
			highbit_tick = HIGHBIT_TICK_200K;
		break;
		
		default:
			highbit_tick = HIGHBIT_TICK_48K;
		break;
	}

	if(idtype == GunID_1Byte)
	{
		//为了防止Buff从带计数转换成不带计数，多出的计数位会影响3字节0
		//故每次重新设置前先将buff清空，重新装载
		memset(IrDMA_1ByteBuff, 0, GUNID_1BYTE_CNT_LEN * 2);//buff是unsigned short类型，故字节数需要×2，仿真才发现错误
		
		IrDMA_1ByteBuff[0] = highbit_tick;
		IrDMA_1ByteBuff[1] = 0;
		IrDMA_1ByteBuff[2] = highbit_tick;
		IrDMA_1ByteBuff[3] = highbit_tick;
		
		gunid = gunid_buff[0];
		for(i=0;i<8;i++)
		{
			bit = (gunid >> i) & 0x01;
			if(bit == 0x01)
			{
				IrDMA_1ByteBuff[4+i] = highbit_tick;
			}
			else
			{
				IrDMA_1ByteBuff[4+i] = 0;
			}
		}
		if(Odd_parity(gunid))
		{
			IrDMA_1ByteBuff[12] = highbit_tick;
		}
		else
		{
			IrDMA_1ByteBuff[12] = 0;
		}
		
		if(cntkey)//判断是否需要加计数位
		{
			IrDMA_1ByteBuff[22] = highbit_tick;
			IrDMA_1ByteBuff[23] = highbit_tick;
			IrDMA_1ByteBuff[24] = 0;
			IrDMA_1ByteBuff[25] = highbit_tick;
			
			IRSendNum = 1;//若需要加计数位，则将计数变量清零，从1开始计数
			DMA_Buff_CntUpdate(1);//设置一下计数位为1时的DMA buff
		}
		else
		{
			IrDMA_1ByteBuff[13] = highbit_tick;
			IrDMA_1ByteBuff[14] = highbit_tick;
			IrDMA_1ByteBuff[15] = 0;
			IrDMA_1ByteBuff[16] = highbit_tick;
		}

	}
	else///3字节枪号
	{
		//为了防止Buff从带计数转换成不带计数，多出的计数位会影响3字节0
		//故每次重新设置前先将buff清空，重新装载
		memset(IrDMA_3ByteBuff, 0, GUNID_3BYTE_CNT_LEN * 2);//buff是unsigned short类型，故字节数需要×2，仿真才发现错误
		
		IrDMA_3ByteBuff[0] = highbit_tick;
		IrDMA_3ByteBuff[1] = 0;
		IrDMA_3ByteBuff[2] = highbit_tick;
		IrDMA_3ByteBuff[3] = highbit_tick;
		
		if(cntkey)//判断是否需要加计数位
		{
			num = 5;
			IRSendNum = 1;//若需要加计数位，则将计数变量清零，从1开始计数
			IRSendNum = gunid_buff[3] = 1;
			gunid_buff[4] = Check_Sum(gunid_buff,4);
			IrDMA_3ByteBuff[49] = highbit_tick;
			IrDMA_3ByteBuff[50] = highbit_tick;
			IrDMA_3ByteBuff[51] = 0;
			IrDMA_3ByteBuff[52] = highbit_tick;
		}
		else
		{
			num = 4;
			gunid_buff[3] = gunid_buff[4];//把校验和放到前一位来，方便转换成Bit数组
			IrDMA_3ByteBuff[40] = highbit_tick;
			IrDMA_3ByteBuff[41] = highbit_tick;
			IrDMA_3ByteBuff[42] = 0;
			IrDMA_3ByteBuff[43] = highbit_tick;
		}
		
		for(i=0;i<num;i++)
		{
			for(j=0;j<8;j++)
			{
				bit = (gunid_buff[i] >> j) & 0x01;
				if(bit == 0x01)
				{
					IrDMA_3ByteBuff[i*9+4+j] = highbit_tick;
				}
				else
				{
					IrDMA_3ByteBuff[i*9+4+j] = 0;
				}
			}
			if(Odd_parity(gunid_buff[i]))
			{
				IrDMA_3ByteBuff[i*9+12] = highbit_tick;
			}
			else
			{
				IrDMA_3ByteBuff[i*9+12] = 0;
			}
		}
		
	}
	
}

/*******************************************************************************
* Function Name  : static void DMA_Buff_CntUpdate(unsigned char cnt)
* Description    : 用于每次计数后更新下次待发送的DMA buff, 当使能了计数时，此函数在Ir_Shot()中调用
* Input          : unsigned char cnt--计数值
* Output         : None
* Return         : None
* Attention		 	 : None
*******************************************************************************/
static void DMA_Buff_CntUpdate(unsigned char cnt)
{
	unsigned char high_tick = 0;
	unsigned char i = 0, j = 0, bit = 0, cnt_val = cnt;
	
	//首先根据激光频率确定需要填充的高电平tick值
	switch(IR_Info.IR_Freq)
	{
		case Freq_48K:
			high_tick = HIGHBIT_TICK_48K;
		break;
		case Freq_500K:
			high_tick = HIGHBIT_TICK_500K;
		break;
		case Freq_200K:
			high_tick = HIGHBIT_TICK_200K;
		break;
		default:
			high_tick = HIGHBIT_TICK_48K;
		break;
	}
	//然后根据枪号类型，选择不同的buff和填充方式
	switch(IR_Info.ID_Type)
	{
		case GunID_1Byte://1字节枪号没有校验和
			for(i=0;i<8;i++)
			{
				bit = (cnt_val >> i) & 0x01;
				if(bit == 0x01)
				{
					IrDMA_1ByteBuff[13+i] = high_tick;
				}
				else
				{
					IrDMA_1ByteBuff[13+i] = 0;
				}
			}
			if(Odd_parity(cnt_val))
			{
				IrDMA_1ByteBuff[21] = high_tick;
			}
			else
			{
				IrDMA_1ByteBuff[21] = 0;
			}
		break;
		
		case GUNID_3Byte://3字节枪号需要每次同时更新校验和的数值
			GunID_Buff[3] =  cnt_val;
			GunID_Buff[4] = Check_Sum(GunID_Buff,4);
			for(i=3;i<5;i++)
			{
				for(j=0;j<8;j++)
				{
					bit = (GunID_Buff[i] >> j) & 0x01;
					if(bit == 0x01)
					{
						IrDMA_3ByteBuff[i*9+4+j] = high_tick;
					}
					else
					{
						IrDMA_3ByteBuff[i*9+4+j] = 0;
					}
				}
				if(Odd_parity(GunID_Buff[i]))
				{
					IrDMA_3ByteBuff[i*9+12] = high_tick;
				}
				else
				{
					IrDMA_3ByteBuff[i*9+12] = 0;
				}
		}
		break;
	}
}

//射弹后上报BLE
static void Shot_BLEReport(unsigned short shot_cnt, unsigned short bullet_remian)
{
	unsigned short crcval = 0;
	unsigned char report_buff[18] = {0};
	
	report_buff[0] = BLE_HEADER;//帧头
	report_buff[1] = RIFLE_SLAVE;//源地址
	report_buff[2] = BLE_MASTER;//源地址
	report_buff[3] = BLE_SHOT_REPROT;//指令码
	report_buff[4] = 10;//数据长度
	
	report_buff[5] = GunID_Buff[0];//MAC
	report_buff[6] = GunID_Buff[1];//MAC
	report_buff[7] = 0;//MAC
	report_buff[8] = 0;//MAC
	report_buff[9] = RIFLE_MAC;//MAC
	report_buff[10] = MAC_HEADER;//MAC
	
	report_buff[11] = bullet_remian >> 8;//剩余子弹数
	report_buff[12] = bullet_remian & 0xff;//剩余子弹数
	report_buff[13] = shot_cnt >> 8;//已击发数
	report_buff[14] = shot_cnt & 0xff;//已击发数
	crcval = CRC_Check(report_buff, 15);
	report_buff[15] = crcval >> 8;
	report_buff[16] = crcval;
	report_buff[17] = BLE_TAIL;
	
	BLE_TX(report_buff, 18);
}

//BLE回复查询激光参数的指令
void IR_InfoQuery_Reply(void)
{
	unsigned short crcval = 0;
	unsigned char reply_buff[15] = {0};
	
	reply_buff[0] = BLE_HEADER;
	reply_buff[1] = RIFLE_SLAVE;//源地址
	reply_buff[2] = BLE_MASTER;//目的地地址
	reply_buff[3] = IR_INFO_QUERY;//指令码
	reply_buff[4] = 0x07;//数据长度
	reply_buff[5] = IR_Info.ID_Type;//枪号位数
	reply_buff[6] = IR_Info.Cnt_Key;//计数开关
	reply_buff[7] = IR_Info.IR_Freq;//激光频率
	reply_buff[8] = IR_Info.Send_Frame;//激光帧数
	memcpy(&reply_buff[9], IR_Info.ID_Buff, 3);//枪号
	crcval = CRC_Check(reply_buff, 12);
	reply_buff[12] = crcval >> 8;
	reply_buff[13] = crcval;
	reply_buff[14] = BLE_TAIL;
	
	BLE_TX(reply_buff, 15);
}

//BLE回复查询枪械参数的指令
void Gun_DevInfo_Reply(void)
{
	unsigned short crcval = 0;
	unsigned char reply_buff[17] = {0};
	
	reply_buff[0] = BLE_HEADER;
	reply_buff[1] = RIFLE_SLAVE;//源地址
	reply_buff[2] = BLE_MASTER;//目的地地址
	reply_buff[3] = BLE_DEV_STAQUERY;//指令码
	reply_buff[4] = 0x09;//数据长度
	reply_buff[5] = Get_Battery();//电量百分比
	reply_buff[6] = Gun_Info.Gun_Lock;//枪械锁
	reply_buff[7] = Gun_Info.Gun_Mode;//射击模式
	reply_buff[8] = Gun_Info.Bullet_Num >> 8;//剩余子弹数
	reply_buff[9] = Gun_Info.Bullet_Num & 0xff;//剩余子弹数
	reply_buff[10] = Gun_Info.Shoot_Cnt >> 8;//击发数
	reply_buff[11] = Gun_Info.Shoot_Cnt & 0xff;//击发数
	reply_buff[12] = Query_TimerPeriod(T_TIMED_SHOOT) >> 8;//定时频率
	reply_buff[13] = Query_TimerPeriod(T_TIMED_SHOOT) & 0xff;//定时频率
	crcval = CRC_Check(reply_buff, 14);
	reply_buff[14] = crcval >> 8;
	reply_buff[15] = crcval;
	reply_buff[16] = BLE_TAIL;
	
	BLE_TX(reply_buff, 17);
}

//BLE设置枪械参数
void GunInfo_Set(unsigned char *buf, unsigned char len)
{
	unsigned short crcval = 0, period_time = 0;
	unsigned char reply_buff[9] = {0};
	unsigned char last_mode = 0xff, new_mode = 0xff;
	
	reply_buff[5] = 0x01;//默认设置成功
	//先判断是不是定时射击模式，如果是，则取出定时频率
	if(buf[5] == Timed_ShootMode)
	{
		period_time = (buf[8] << 8) + buf[9];
		if(period_time < 100)
		{
			reply_buff[5] = 0x00;//如果设置小于100ms,则设置失败
		}
		else
		{
			Change_TimerPeriod(T_TIMED_SHOOT, period_time);//如果是正确设置，则直接改变软件定时器周期
		}
	}
	if(buf[5] == Calibrate_Mode)//如果是校准模式，则创建软件定时器
	{
		CreatTimer(T_IR_CALIBRATE, IR_Calibrate_Proc, 200, T_STOP_STA);//定时射击模式
	}
	//如果不是定时射击模式，再判断是否是正确的模式设置
	if((buf[5] <= 4) && (reply_buff[5] == 0x01))
	{
		last_mode = Gun_Info.Gun_Mode;//保存旧模式
		new_mode = Gun_Info.Gun_Mode = buf[5];//更新枪械模式
		Gun_Info.Shoot_Cnt = 0;//从0开始计数
		//子弹数每800ms都在检查更新，故这里不去主动刷屏了
		Gun_Info.Bullet_Num = (buf[6] << 8) + buf[7];
		reply_buff[5] = 0x01;//设置成功
		
		//准备开始比赛，先锁枪，等待开始训练指令再解锁
		Gun_Info.Gun_Lock = Locked_Gun;
		Machine_LockGun();//机械锁枪
		Set_MenuPosition(2);
		if(last_mode != new_mode)//新旧状态不一样，则需要更新状态
		{
			if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//如果目前没在设备信息页面，那么可以不主动刷屏
			{
				Set_ClrScreenFlag();
			}
		}
	}
	else
	{
		reply_buff[5] = 0x00;//设置失败
	}
	
	if(reply_buff[5] == 0x01)//如果成功设置，那么就将枪号设置为正常枪号
	{
		Mode_Change_GunID();
	}
	delay_ms(20);
	//BLE回复
	reply_buff[0] = BLE_HEADER;
	reply_buff[1] = RIFLE_SLAVE;//源地址
	reply_buff[2] = BLE_MASTER;//目的地地址
	reply_buff[3] = BLE_DEV_INFOSET;//指令码
	reply_buff[4] = 0x01;//数据长度
	crcval = CRC_Check(reply_buff, 6);
	reply_buff[6] = crcval >> 8;
	reply_buff[7] = crcval;
	reply_buff[8] = BLE_TAIL;
	
	BLE_TX(reply_buff, 9);
}

//BLE设置枪械开关
void Gun_LockSet(unsigned char key)
{
	unsigned short crcval = 0;
	unsigned char reply_buff[9] = {0};
	unsigned char last_lock = 0xff, new_lock = 0xff;
	
	last_lock = Gun_Info.Gun_Lock;//保存一下之前的锁枪状态
	//先设置枪锁
	if(key == 0)
	{
		new_lock = Gun_Info.Gun_Lock = Locked_Gun;//更新锁枪状态
		Machine_LockGun();
		reply_buff[5] = 1;//设置成功
	}
	else if(key == 1)
	{
		new_lock = Gun_Info.Gun_Lock = Unlocked_Gun;//更新锁枪状态
		Machine_UnlockGun();
		reply_buff[5] = 1;//设置成功
	}
	else
	{
		reply_buff[5] = 0;//设置不成功
	}
	
	if(last_lock != new_lock)//如果锁枪状态变化了，那就刷新屏幕
	{
		if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//如果目前没在设备信息页面，那么可以不主动刷屏
		{
			Set_ClrScreenFlag();
		}
	}
	
	//回复BLE
	reply_buff[0] = BLE_HEADER;
	reply_buff[1] = RIFLE_SLAVE;//源地址
	reply_buff[2] = BLE_MASTER;//目的地地址
	reply_buff[3] = BLE_GUN_SWITCH;//指令码
	reply_buff[4] = 0x01;//数据长度
	crcval = CRC_Check(reply_buff, 6);
	reply_buff[6] = crcval >> 8;
	reply_buff[7] = crcval;
	reply_buff[8] = BLE_TAIL;
	
	BLE_TX(reply_buff, 9);
}

//BLE补充枪械子弹
void Gun_BulletAdd(unsigned char *buf, unsigned char len)
{
	unsigned short crcval = 0, bullet_last = 0, bullet_add = 0;
	unsigned char reply_buff[9] = {0};
	
	//补充子弹
	bullet_add = (buf[5] << 8) + buf[6];
	bullet_last = 65535 - Gun_Info.Bullet_Num;//计算还有多少子弹就满了
	if(bullet_add > bullet_last)
	{
		Gun_Info.Bullet_Num = 65535;//若会超出65535发，那么就给子弹加满为65535发
	}
	else
	{
		Gun_Info.Bullet_Num += bullet_add;//不会超出的话，就直接给他加上
	}
	
	reply_buff[0] = BLE_HEADER;
	reply_buff[1] = RIFLE_SLAVE;//源地址
	reply_buff[2] = BLE_MASTER;//目的地地址
	reply_buff[3] = BLE_BULLET_ADD;//指令码
	reply_buff[4] = 0x01;//数据长度
	reply_buff[5] = 0x01;//设置成功
	crcval = CRC_Check(reply_buff, 6);
	reply_buff[6] = crcval >> 8;
	reply_buff[7] = crcval;
	reply_buff[8] = BLE_TAIL;
	
	BLE_TX(reply_buff, 9);
}

//用于查询枪械模式
unsigned char Query_GunMode(void)
{
	if(Gun_Info.Gun_Mode == Dev_TestMode)
	{
		return 0;
	}
	else if(Gun_Info.Gun_Mode == Unlimited_BulletMode)
	{
		return 1;
	}
	else if(Gun_Info.Gun_Mode == Timed_ShootMode)
	{
		return 2;
	}
	else if(Gun_Info.Gun_Mode == Normal_ShootMode)
	{
		return 3;
	}
	else if(Gun_Info.Gun_Mode == Calibrate_Mode)
	{
		return 4;
	}
	
	return 0xff;
}

//用来查询锁枪状态
unsigned char Query_GunLock(void)
{
	if(Gun_Info.Gun_Lock == Locked_Gun)
	{
		return 0;
	}
	else if(Gun_Info.Gun_Lock == Unlocked_Gun)
	{
		return 1;
	}
	
	return 0xff;
}

//查询剩余子弹数
unsigned short Query_BulletNum(void)
{
	return Gun_Info.Bullet_Num;
}

//查询击发数
unsigned short Query_ShootCnt(void)
{
	return Gun_Info.Shoot_Cnt;
}

//暂时用不到，复位本次上电后激光发射次数
void Reset_IRNum(void)
{
	IRSendNum = 1;//TODO:此处需要增加波形更新,当波形带计数位就需要同时更新波形
}

//BLE指令开始训练
void Start_Train(void)//TODO:加入蜂鸣器鸣响的指示
{
	//锁枪
	Gun_Info.Gun_Lock = Unlocked_Gun;
	Machine_UnlockGun();//机械开锁
	//Gun_Info.Gun_Mode = Normal_ShootMode;//改变状态为射击模式//不必改变模式了
	if(Gun_Info.Gun_Mode == Calibrate_Mode)
	{
		ResetTimer(T_IR_CALIBRATE, T_START_STA);//开启校准模式
	}
	
	if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//更新菜单页面
	{
		Set_ClrScreenFlag();
	}
	Gun_Info.Shoot_Cnt = 0;//从0开始计数
	if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//更新菜单页面
	{
		Set_ClrScreenFlag();
	}
	BEEP_TrainStart();//蜂鸣器短响3声
	Change_TrainSta(1);//开始训练
	ScreenControl(0);//熄灭屏幕
}

//BLE指令设备复位
void Reset_Dev(void)//TODO:加入蜂鸣器的操作
{
	//复位成刚开机时的状态
	Gun_Info.Gun_Lock = Unlocked_Gun;
	Machine_UnlockGun();//机械开锁
	if(Gun_Info.Gun_Mode == Calibrate_Mode)
	{
		CtrlTimer(T_IR_CALIBRATE, T_STOP_STA);//如果现在处于校准模式，那就先关闭软件定时器
	}
	Gun_Info.Gun_Mode = Dev_TestMode;//测试模式
	if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//更新菜单页面
	{
		Set_ClrScreenFlag();
	}
	Gun_Info.Bullet_Num = 30;//赋给30发子弹
	Gun_Info.Shoot_Cnt = 0;//从0开始计数
	if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//更新菜单页面
	{
		Set_ClrScreenFlag();
	}
	BEEP_PowerOn();//跟上下电一样，短响一声
	if(Query_TrainSta() == Train_OnSta)//如果处于训练状态，则退出训练，亮屏
	{
		Change_TrainSta(0);//退出训练
		ScreenControl(1);//点亮屏幕
	}
}

//BLE指令结束训练
void Finish_Train(void)//TODO:加入蜂鸣器鸣叫的操作
{
	//锁枪
	Gun_Info.Gun_Lock = Locked_Gun;
	Machine_LockGun();//机械锁枪
	if(Gun_Info.Gun_Mode == Calibrate_Mode)
	{
		CtrlTimer(T_IR_CALIBRATE, T_STOP_STA);//如果现在处于校准模式，那就先关闭软件定时器
	}
	Gun_Info.Gun_Mode = Dev_TestMode;//复位为测试模式
	BEEP_TrainEnd();//训练结束短响6声
	Change_TrainSta(0);//退出训练
	ScreenControl(1);//点亮屏幕
	if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//更新菜单页面
	{
		Set_ClrScreenFlag();
	}
	Gun_Info.Bullet_Num = 0;//清空可用子弹
	if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//更新菜单页面
	{
		Set_ClrScreenFlag();
	}
	
}


///校准射击模式的处理函数
void IR_Calibrate_Proc(void)
{
	Ir_Shot();
	ResetTimer(T_IR_CALIBRATE,T_START_STA);
}


