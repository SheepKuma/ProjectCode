#include "sys.h" 
#include "app.h"
#include "led.h"
#include "key.h"
#include "ir.h"
#include "ble_usart.h"
#include "usart.h"
#include "e2prom.h"
#include "delay.h"
#include "oled.h"
#include "mytimer.h"
#include "motor_ms3111s.h"
#include "stepper_motor.h"
#include "beep.h"
#include "adc.h"
#include "mytimer2.h"


static void AppInfo_Init(void);
//static void Set_LightSize(unsigned char size);
static void IR_InfoSet_Reply(void);
static void KeyEventHandle(KEY_VALUE_TYPEDEF keys);
static void menuInit(void);
static void gnlMenu_DesktopCBS(void);
static void gnlMenu_DevInfoCBS(void);
static void gnlMenu_ShootInfoCBS(void);
//static void BLE_LightSize_Change(unsigned char size);
//static void Query_Light_Size(void);
static void BLE_MAC_QueryHandle(unsigned char *buf, unsigned char len);



//unsigned char writeAry[66] = {0},readAry[66] = {0},i=0;///测试I2C读写E2PROM
//unsigned char Stepper_Motor[7] = {0};//测试步进电机的I2C读写

TRAIN_INFO_STU Train_Info = {0};

stu_mode_menu *pModeMenu;		//系统当前菜单
//初始化桌面菜单
stu_mode_menu generalModeMenu[GNL_MENU_SUM] =
{
	{GNL_MENU_DESKTOP, DESKTOP_MENU_POS, "Desktop", gnlMenu_DesktopCBS, SCREEN_CMD_RESET, 0, 0xFF, 0, 0, 0, 0},
	{GNL_DEV_INFO, DEV_INFO_POS, "DevInfo", gnlMenu_DevInfoCBS, SCREEN_CMD_RESET, 0, 0xFF, 0, 0, 0, 0},
	{GNL_SHOOT_INFO, SHOOT_INFO_POS, "ShootInfo", gnlMenu_ShootInfoCBS, SCREEN_CMD_RESET, 0, 0xFF, 0, 0, 0, 0},
};	

//刷新屏幕的标志位, =0不需要刷新，=1需要刷新
unsigned char ClrScreenFlag = 0;
//配对的允许标志位，为1时允许配对，为0时不允许配对，用于上电后3S内允许配对
//unsigned char PairKey_PowerON = 0;//改为APP配对

void APP_Init(void)
{
	KeyScan_CBSRegister(KeyEventHandle);//按键回调
	e2prom_Init();//E2PROM

	ADC_PowerON();//电量采集
	//OLED屏幕初始化
	OLED_Init();
	OLED_ColorTurn(0);//0正常显示，1 反色显示
  OLED_DisplayTurn(1);//0正常显示 1 屏幕翻转显示
	menuInit();//菜单初始化
	//改为结构上手动调节，故软件上取消电机控制
	//Stepper_Motor_Init();//步进电机
	
	AppInfo_Init();
	
	CreatTimer(T_APP, App_Proc, 10, T_START_STA);//10ms轮询一次菜单
	CreatTimer(T_APP_REFRESH_GUNNUM, Refresh_GunNum, 800, T_START_STA);//800ms轮询一次是否需要刷新射弹数
	CreatTimer(T_TIMED_SHOOT, IR_Proc, Train_Info.Timed_Period, T_STOP_STA);//定时射击模式
	CreatTimer(T_STEPPER_CLOSE,Stepper_Motor_Close,5000,T_STOP_STA);//电机运行5秒后关机
}

//初始化APP相关的参数
static void AppInfo_Init(void)
{
	//unsigned char size_read = 0;
	//先读取光斑大小
	//改为手动调节，故软件上取消控制
	/*I2C_Read(10, (unsigned char *)&size_read, 1);//光斑大小存放在E2PROM的地址10
	if(size_read > 1)
	{
		Train_Info.Light_Size = size_read = Target_Mode;//如果是第一次上电，则初始化为小光斑
		I2C_PageWrite(10, (unsigned char *)&size_read, 1);
	}
	else
	{
		Train_Info.Light_Size = size_read;//不是第一次上电，则赋给结构体变量
	}*/
	Train_Info.Light_Size = Target_Mode;//先留着
	ClrScreenFlag = 0;//刷新屏幕的标志位
	Train_Info.Train_Sta = Train_OffSta;//上电默认为结束训练状态
	Train_Info.Screen_Sta = 1;//亮屏
	Train_Info.Timed_Period = 200;//定时射击模式的周期上电后默认是200ms的周期
	Train_Info.TurnOff_Screen_Cnt = 0;//初始化为0
}

///定时射击模式的处理函数
void IR_Proc(void)
{
	Ir_Shot();
	ResetTimer(T_TIMED_SHOOT,T_START_STA);
}

//设置光斑大小
//改为手动调节
/*
static void Set_LightSize(unsigned char size)
{
	if(size != Train_Info.Light_Size)
	{
		Train_Info.Light_Size = size;
		I2C_PageWrite(10, (unsigned char *)&(Train_Info.Light_Size), 1);
		switch(size)//TODO:确认正转反转是否是正确的光斑变大变小方向
		{
			case Target_Mode:
				Stepper_Motor_Open();
				delay_ms(10);

				M_I2C_WriteByte(0x20, 0x00, 0x21);  //寄存器使能通道打开
				///0x40--32细分，正向；0x41--32细分，反向；0x00--256细分，正向；0x01--256细分，反向
			///0xE0--1步进，正向；0xE1--1步进，反向；0xA0--4细分，正向；0xA1--4细分，反向
				M_I2C_WriteByte(0x20,0x01,0x01);     //换向、细分数据
				//运行频率见DEMO
				M_I2C_WriteByte(0x20,0x02,0x00);     //运行频率
				M_I2C_WriteByte(0x20,0x03,0x05);     //运行频率
				//根据步距角0.047°计算步进数--180°是0X3BD7,10°是0x353
				M_I2C_WriteByte(0x20,0x04,0x53);//0xD7     //步数
				M_I2C_WriteByte(0x20,0x05,0x03);//0x3B     //步数
				M_I2C_WriteByte(0x20, 0x06, 0x4D);     //运行电流60%-0x4D,100%-0x7F,140%-0xBF
				M_I2C_WriteByte(0x20, 0x07, 0x00);     //保持电流30%-0x26,100-0x7F,0-0x00
				M_I2C_WriteByte(0x20, 0x08, 0x0F);     //等待15ms进入保持电流-0x0F,255ms-0xFF
				M_I2C_WriteByte(0x20,0x09,0x01);     //程序载入、急停
				ResetTimer(T_STEPPER_CLOSE, T_START_STA);//5s后关闭步进电机
			break;
			case Confront_Mode:
				Stepper_Motor_Open();
				delay_ms(10);

				M_I2C_WriteByte(0x20, 0x00, 0x21);  //寄存器使能通道打开
				///0x40--32细分，正向；0x41--32细分，反向；0x00--256细分，正向；0x01--256细分，反向
			///0xE0--1步进，正向；0xE1--1步进，反向；0xA0--4细分，正向；0xA1--4细分，反向
				M_I2C_WriteByte(0x20,0x01,0x00);     //换向、细分数据
				//运行频率见DEMO
				M_I2C_WriteByte(0x20,0x02,0x00);     //运行频率
				M_I2C_WriteByte(0x20,0x03,0x05);     //运行频率
				//根据步距角0.047°计算步进数--180°是0X3BD7,10°是0x353
				M_I2C_WriteByte(0x20,0x04,0x53);//0xD7     //步数
				M_I2C_WriteByte(0x20,0x05,0x03);//0x3B     //步数
				M_I2C_WriteByte(0x20, 0x06, 0x4D);     //运行电流60%-0x4D,100%-0x7F,140%-0xBF
				M_I2C_WriteByte(0x20, 0x07, 0x00);     //保持电流30%-0x26,100-0x7F,0-0x00
				M_I2C_WriteByte(0x20, 0x08, 0x0F);     //等待15ms进入保持电流-0x0F,255ms-0xFF
				M_I2C_WriteByte(0x20,0x09,0x01);     //程序载入、急停
				ResetTimer(T_STEPPER_CLOSE, T_START_STA);//5s后关闭步进电机
			break;
		}
		if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//更新菜单页面
		{
			Set_ClrScreenFlag();
		}
	}
}*/

//TEST
//改为手动调节
/*static void Query_Light_Size(void)
{
	unsigned char reply[3] = {0};
	reply[0] = 0x68;
	I2C_Read(10, (unsigned char *)&reply[1], 1);//光斑大小存放在E2PROM的地址10
	reply[2] = 0x16;
	BLE_TX(reply, 3);
}*/


//控制屏幕亮灭，cmd-0熄屏，-1亮屏
void ScreenControl(unsigned char cmd)
{
	if(cmd)
	{
		if(Train_Info.Screen_Sta == 0)//熄屏中
		{
			Train_Info.Screen_Sta = 1;
			OLED_DisPlay_On();//点亮屏幕
			Train_Info.TurnOff_Screen_Cnt = 0;//计数清零
		}
	}
	else
	{
		if(Train_Info.Screen_Sta == 1)//亮屏中
		{
			Train_Info.Screen_Sta = 0;
			OLED_DisPlay_Off();//熄灭屏幕
			Train_Info.TurnOff_Screen_Cnt = 0;//计数清零
		}
	}
}

//改变训练状态,cmd-0退出训练，-1进入训练
void Change_TrainSta(unsigned char cmd)
{
	if(cmd)
	{
		Train_Info.Train_Sta =Train_OnSta;//进入训练状态
		Train_Info.TurnOff_Screen_Cnt = 0;
	}
	else
	{
		Train_Info.Train_Sta =Train_OffSta;//退出训练状态
		Train_Info.TurnOff_Screen_Cnt = 0;
		//结束训练亮屏后直接显示子弹页面
		pModeMenu = &generalModeMenu[GNL_SHOOT_INFO];
		pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
	}
}
//查询训练状态
unsigned char Query_TrainSta(void)
{
	return Train_Info.Train_Sta;
}

void App_Proc(void)
{
	pModeMenu->action();
	//处于训练开始状态，且亮屏时就计数，大于5秒就熄屏
	if((Train_Info.Train_Sta == Train_OnSta) && (Train_Info.Screen_Sta  == 1))
	{
		Train_Info.TurnOff_Screen_Cnt ++;
		if(Train_Info.TurnOff_Screen_Cnt > PUTOUT_SCREEN_PERIOD)
		{
			Train_Info.TurnOff_Screen_Cnt = 0;
			ScreenControl(0);//5S无操作熄屏
		}
	}
	ResetTimer(T_APP,T_START_STA);
}

////////////菜单相关处理/////////////////
static void menuInit(void)
{
	pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];	//设置上电显示的菜单界面为桌面显示
	pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;	//更新刷新界面标志，进入界面后刷新全界面UI
}
//桌面菜单的回调函数
static void gnlMenu_DesktopCBS(void)
{
	unsigned char keys;
	static unsigned char adc_per = 0;
	
	if(Query_ClrScreenFlag())
	{
		Reset_ClrScreenFlag();
		pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
	}
	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		pModeMenu->keyVal = 0xFF;
		
		OLED_Clear();
		//16×16
		//主标题：“步枪激光下挂”
//		OLED_ShowChinese(16,0,0x00,16,1);
//		OLED_ShowChinese(32,0,0x01,16,1);
//		OLED_ShowChinese(48,0,0x11,16,1);
//		OLED_ShowChinese(64,0,0x12,16,1);
//		OLED_ShowChinese(80,0,0x02,16,1);
//		OLED_ShowChinese(96,0,0x03,16,1);
		//Sheep: 间距大些好看吧
//		OLED_ShowChinese(11,0,0x00,16,1);
//		OLED_ShowChinese(29,0,0x01,16,1);
//		OLED_ShowChinese(47,0,0x11,16,1);
//		OLED_ShowChinese(65,0,0x12,16,1);
//		OLED_ShowChinese(83,0,0x02,16,1);
//		OLED_ShowChinese(101,0,0x03,16,1);
		OLED_ShowChinese(14,0,0x00,16,1);
		OLED_ShowChinese(31,0,0x01,16,1);
		OLED_ShowChinese(48,0,0x11,16,1);
		OLED_ShowChinese(65,0,0x12,16,1);
		OLED_ShowChinese(82,0,0x02,16,1);
		OLED_ShowChinese(99,0,0x03,16,1);
		
		//电量显示：“电量:55%”
		adc_per = Get_Battery();
		OLED_ShowString(24,16,(u8 *)"-",16,1);
		OLED_ShowChinese(33,16,0x04,16,1);//25
		OLED_ShowChinese(49,16,0x05,16,1);//41
		if(adc_per >= 100)
		{
//			OLED_ShowString(57,16,(u8 *)": 100%",16,1);
			OLED_ShowString(65,16,(u8 *)":100%",16,1);//57
			OLED_ShowString(106,16,(u8 *)"-",16,1);
		}
		else if(adc_per < 10)
		{
//			OLED_ShowString(57,16,(u8 *)": ",16,1);
//			OLED_ShowChar(73,16,(adc_per%10)+'0',16,1);
//			OLED_ShowString(81,16,(u8 *)"%",16,1);
			OLED_ShowString(65,16,(u8 *)":",16,1);//57
			OLED_ShowChar(73,16,(adc_per%10)+'0',16,1);//65
			OLED_ShowString(81,16,(u8 *)"%",16,1);//73
			OLED_ShowString(89,16,(u8 *)"-",16,1);
		}
		else
		{
//			OLED_ShowString(57,16,(u8 *)": ",16,1);
//			OLED_ShowChar(73,16,(adc_per/10)+'0',16,1);
//			OLED_ShowChar(81,16,(adc_per%10)+'0',16,1);
//			OLED_ShowString(89,16,(u8 *)"%",16,1);
			OLED_ShowString(65,16,(u8 *)":",16,1);//57
			OLED_ShowChar(73,16,(adc_per/10)+'0',16,1);//65
			OLED_ShowChar(81,16,(adc_per%10)+'0',16,1);//73
			OLED_ShowString(89,16,(u8 *)"%",16,1);//81
			OLED_ShowString(100,16,(u8 *)"-",16,1);
		}
		
		OLED_Refresh();
	}
	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		 
		pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
		switch(keys)
		{
			case KEY_SET_CLICK_RELEASE:
				pModeMenu = &generalModeMenu[GNL_DEV_INFO];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
		}
	}
	
}


static void gnlMenu_DevInfoCBS(void)
{
	unsigned char keys;
	unsigned char mode = 0xff, lock = 0xff;
	
	if(Query_ClrScreenFlag())
	{
		Reset_ClrScreenFlag();
		pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
	}
	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		keys = 0xFF;
		
		OLED_Clear();
		//模式标题：“打靶模式”
		///由于结构上取消了电机，改为手动调节，软件上检测不到光斑的模式，故菜单不显示相关信息了
		/*if(Train_Info.Light_Size == Target_Mode)//打靶模式
		{
			OLED_ShowString(0,0,(u8 *)"* ",16,1);
			OLED_ShowChinese(32,0,0x0A,16,1);
			OLED_ShowChinese(48,0,0x0B,16,1);
			OLED_ShowChinese(64,0,0x0C,16,1);
			OLED_ShowChinese(80,0,0x0D,16,1);
		}
		else if(Train_Info.Light_Size == Confront_Mode)//对抗模式
		{
			OLED_ShowString(0,0,(u8 *)"* ",16,1);
			OLED_ShowChinese(32,0,0x24,16,1);
			OLED_ShowChinese(48,0,0x25,16,1);
			OLED_ShowChinese(64,0,0x0C,16,1);
			OLED_ShowChinese(80,0,0x0D,16,1);
		}*/
		
		//枪械模式与锁枪状态：“普通射击 - 上锁”
		//枪械模式的显示更新处理在BLE设置枪械参数的处理函数GunInfo_Set()函数中进行
		mode = Query_GunMode();
		//OLED_ShowString(0,17,(u8 *)"* ",16,1);
		switch(mode)
		{
			case Dev_TestMode:
				OLED_ShowChinese(16,0,0x13,16,1);//4
				OLED_ShowChinese(32,0,0x14,16,1);//20
				OLED_ShowChinese(48,0,0x15,16,1);//36
				OLED_ShowChinese(64,0,0x16,16,1);//52
				OLED_ShowChinese(80,0,0x0C,16,1);
				OLED_ShowChinese(96,0,0x0D,16,1);
			break;
			case Unlimited_BulletMode:
				OLED_ShowChinese(16,0,0x17,16,1);
				OLED_ShowChinese(32,0,0x18,16,1);
				OLED_ShowChinese(48,0,0x19,16,1);
				OLED_ShowChinese(64,0,0x1A,16,1);
				OLED_ShowChinese(80,0,0x0C,16,1);
				OLED_ShowChinese(96,0,0x0D,16,1);
			break;
			case Timed_ShootMode:
				OLED_ShowChinese(16,0,0x19,16,1);
				OLED_ShowChinese(32,0,0x1A,16,1);
				OLED_ShowChinese(48,0,0x2B,16,1);
				OLED_ShowChinese(64,0,0x09,16,1);
				OLED_ShowChinese(80,0,0x0C,16,1);
				OLED_ShowChinese(96,0,0x0D,16,1);
			break;
			case Normal_ShootMode:
				OLED_ShowChinese(16,0,0x19,16,1);
				OLED_ShowChinese(32,0,0x1A,16,1);
				OLED_ShowChinese(48,0,0x2A,16,1);
				OLED_ShowChinese(64,0,0x09,16,1);
				OLED_ShowChinese(80,0,0x0C,16,1);
				OLED_ShowChinese(96,0,0x0D,16,1);
			break;
			case Calibrate_Mode:
				OLED_ShowChinese(32,0,0x26,16,1);
				OLED_ShowChinese(48,0,0x01,16,1);
				OLED_ShowChinese(64,0,0x0C,16,1);
				OLED_ShowChinese(80,0,0x0D,16,1);
			break;
		}
		//锁枪状态的显示更新处理在BLE设置锁枪的处理函数Gun_LockSet()函数中进行
		OLED_ShowString(31,16,(u8 *)"-",16,1);
		lock = Query_GunLock();
		if(lock == Locked_Gun)
		{
			//OLED_ShowChinese(24,16,0x01,16,1);
			//OLED_ShowChinese(40,16,0x28,16,1);
			//OLED_ShowChinese(56,16,0x0E,16,1);
			OLED_ShowChinese(40,16,0x21,16,1);
			OLED_ShowChinese(56,16,0x22,16,1);
			OLED_ShowChinese(72,16,0x29,16,1);
		}
		else if(lock == Unlocked_Gun)
		{
			//OLED_ShowChinese(24,16,0x01,16,1);
			//OLED_ShowChinese(40,16,0x28,16,1);
			//OLED_ShowChinese(56,16,0x0E,16,1);
			OLED_ShowChinese(40,16,0x0E,16,1);
			OLED_ShowChinese(56,16,0x23,16,1);
			OLED_ShowChinese(72,16,0x22,16,1);
		}
		OLED_ShowString(89,16,(u8 *)"-",16,1);

		OLED_Refresh();
	}
	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		 
		pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
		switch(keys)
		{
			case KEY_SET_CLICK_RELEASE:
				pModeMenu = &generalModeMenu[GNL_SHOOT_INFO];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
		}
	}
}

//射击信息菜单页面
static void gnlMenu_ShootInfoCBS(void)
{
	unsigned char keys;
	unsigned short Bullet = 0, Cnt = 0;
	
	if(Query_ClrScreenFlag())
	{
		Reset_ClrScreenFlag();
		pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
	}
	if(pModeMenu->refreshScreenCmd == SCREEN_CMD_RESET)
	{
		pModeMenu->refreshScreenCmd = SCREEN_CMD_NULL;
		keys = 0xFF;
		
		Cnt = Query_ShootCnt();
		OLED_Clear();
		//已击发显示：“击发：65535发”
		OLED_ShowString(0,0,(u8 *)"* ",16,1);
		OLED_ShowChinese(16,0,0x0F,16,1);
		OLED_ShowChinese(32,0,0x10,16,1);
		OLED_ShowString(48,0,(u8 *)": ",16,1);
		OLED_ShowChar(64,0,(Cnt/10000)+'0',16,1);
		OLED_ShowChar(72,0,(Cnt%10000/1000)+'0',16,1);
		OLED_ShowChar(80,0,(Cnt%1000/100)+'0',16,1);
		OLED_ShowChar(88,0,(Cnt%100/10)+'0',16,1);
		OLED_ShowChar(96,0,(Cnt%10)+'0',16,1);
		OLED_ShowChinese(104,0,0x09,16,1);
		
		//子弹数显示：“子弹：65535发”
		Bullet = Query_BulletNum();
		OLED_ShowString(0,16,(u8 *)"* ",16,1);
		OLED_ShowChinese(16,16,0x19,16,1);
		OLED_ShowChinese(32,16,0x1A,16,1);
		OLED_ShowString(48,16,(u8 *)": ",16,1);
		//处于无限子弹模式时，就显示“子弹： 无限”
		if(Query_GunMode() == Unlimited_BulletMode)
		{
			OLED_ShowChinese(64,16,0x17,16,1);
			OLED_ShowChinese(80,16,0x18,16,1);
		}
		else//其他模式再显示具体子弹数
		{
			OLED_ShowChar(64,16,(Bullet/10000)+'0',16,1);
			OLED_ShowChar(72,16,(Bullet%10000/1000)+'0',16,1);
			OLED_ShowChar(80,16,(Bullet%1000/100)+'0',16,1);
			OLED_ShowChar(88,16,(Bullet%100/10)+'0',16,1);
			OLED_ShowChar(96,16,(Bullet%10)+'0',16,1);
			OLED_ShowChinese(104,16,0x09,16,1);
		}
		
		OLED_Refresh();
	}
	if(pModeMenu->keyVal != 0xff)
	{
		keys = pModeMenu->keyVal;
		 
		pModeMenu->keyVal = 0xFF;	//恢复菜单按键值
		switch(keys)
		{
			case KEY_SET_CLICK_RELEASE:
				pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
		}
	}
}

//置起刷新屏幕的标志位
void Set_ClrScreenFlag(void)
{
	ClrScreenFlag = 1;
}

//清除刷新屏幕的标志位
void Reset_ClrScreenFlag(void)
{
	ClrScreenFlag = 0;
}

//查询刷新屏幕的标志位是否被置起
unsigned char Query_ClrScreenFlag(void)
{
	return ClrScreenFlag;
}

//查询当前显示的是哪个菜单页面
unsigned char Query_MenuPosition(void)
{
	if(pModeMenu ->menuPos == DESKTOP_MENU_POS)
	{
		return 0;
	}
	else if(pModeMenu ->menuPos == DEV_INFO_POS)
	{
		return 1;
	}
	else if(pModeMenu ->menuPos == SHOOT_INFO_POS)
	{
		return 2;
	}
	
	return 0xff;
}

//设置菜单页面显示在某个页面
void Set_MenuPosition(unsigned char page)
{
	if(page == DESKTOP_MENU_POS)
	{
		pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];
		pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
	}
	else if(page == DEV_INFO_POS)
	{
		pModeMenu = &generalModeMenu[GNL_DEV_INFO];
		pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
	}
	else if(page == SHOOT_INFO_POS)
	{
		pModeMenu = &generalModeMenu[GNL_SHOOT_INFO];
		pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
	}
}

//查询射弹数是否要刷新
void Refresh_GunNum(void)
{
	//子弹数
	static unsigned char last_num = 0;
	unsigned char current_num = 0;
	
//	if((Query_GunMode() == Dev_TestMode) || (Query_GunMode() == Normal_ShootMode))//当处于测试设备模式和普通射击模式时才检查子弹数
	if((Query_GunMode() != Unlimited_BulletMode) && (Query_GunMode() != Calibrate_Mode))//只要不是无限子弹模式和校准模式，就处理子弹数
	{
		current_num = Query_BulletNum();
		if(last_num != current_num)
		{
			last_num = current_num;
			if((Query_MenuPosition() == SHOOT_INFO_POS) && (!Query_ClrScreenFlag()))
			{
				Set_ClrScreenFlag();
			}
		}
	}
	else if(Query_GunMode() == Unlimited_BulletMode)//无限子弹模式下就只判断击发数，校准模式则什么都不管
	{
		current_num = Query_ShootCnt();
		if(last_num != current_num)
		{
			last_num = current_num;
			if((Query_MenuPosition() == SHOOT_INFO_POS) && (!Query_ClrScreenFlag()))
			{
				Set_ClrScreenFlag();
			}
		}
	}
	
	ResetTimer(T_APP_REFRESH_GUNNUM,T_START_STA);
}

//////////BLE 数据处理/////////////
void BLE_DataHandle(unsigned char *dat, int len)
{
	unsigned char cmd = dat[3];
	
	switch(cmd)
	{
		case IR_INFO_SET://8.5设置激光参数
			IR_InfoSet_Reply();
			GunID_Change(dat[5], dat[6], dat[7], dat[8], &dat[9]);
		break;
		case IR_INFO_QUERY://8.6查询激光参数
			IR_InfoQuery_Reply();
		break;
		case BLE_TRAIN_START://8.2开始训练
			Start_Train();
		break;
		case BLE_TRAIN_END://8.3结束训练
			Finish_Train();
		break;
		case BLE_DEV_RESET://8.4设备复位
			Reset_Dev();
		break;
		case BLE_DEV_STAQUERY://8.11查询设备状态
			Gun_DevInfo_Reply();
		break;
		case BLE_DEV_INFOSET://8.12设置设备状态
			GunInfo_Set(dat, len);
		break;
		case BLE_GUN_SWITCH://8.13枪械开关
			Gun_LockSet(dat[5]);
		break;
		case BLE_BULLET_ADD://8.15子弹补充
			Gun_BulletAdd(dat, len);
		break;
		case BLE_LIGHT_SIZE://8.18设置光斑大小
			//BLE_LightSize_Change(dat[5]);
		break;
		case BLE_MAC_QUERY://从机BLE模块的MAC地址被主机更改后会通过0xD0指令串口通知STM32改变枪号
			BLE_MAC_QueryHandle(dat, len);
		break;
		case BLE_FIND_DEV://显控查找设备，从机收到后不回复，蜂鸣器鸣叫500ms
			BEEP_FindDev();
		break;
		case SHEEP_TEST:
			/*IR_InfoQuery_Reply();
			delay_ms(10);
			Query_Light_Size();*/
			break;
	}
	
}

static void IR_InfoSet_Reply(void)
{
	unsigned short crcval = 0;
	unsigned char reply_buff[9] = {0};
	
	reply_buff[0] = BLE_HEADER;
	reply_buff[1] = RIFLE_SLAVE;//源地址
	reply_buff[2] = BLE_MASTER;//目的地地址
	reply_buff[3] = IR_INFO_SET;//指令码
	reply_buff[4] = 0x01;//数据长度
	reply_buff[5] = 0x01;//设置成功
	crcval = CRC_Check(reply_buff, 6);
	reply_buff[6] = crcval >> 8;
	reply_buff[7] = crcval;
	reply_buff[8] = BLE_TAIL;
	
	BLE_TX(reply_buff, 9);
}

void BLE_Mac_Change(unsigned char *id_buff)
{
	unsigned short crcval = 0;
	unsigned char send_buff[14] = {0};
	
	send_buff[0] = BLE_HEADER;
	send_buff[1] = 0xFD;//源地址，这个随便写，因为蓝牙模块不校验
	send_buff[2] = 0xFE;//目的地址，这个随便写，因为蓝牙模块不校验
	send_buff[3] = BLE_MAC_CHANGE;//指令码
	send_buff[4] = 0x06;//数据长度
	send_buff[5] = 0xC1;//MAC
	send_buff[6] = RIFLE_MAC;//MAC--训练服成员识别位
	send_buff[7] = 0x00;//MAC
	send_buff[8] = 0x00;//MAC
	send_buff[9] = id_buff[1];//MAC
	send_buff[10] = id_buff[0];//MAC
	crcval = CRC_Check(send_buff, 11);
	send_buff[11] = crcval >> 8;
	send_buff[12] = crcval;
	send_buff[13] = BLE_TAIL;
	
	BLE_TX(send_buff, 14);
}
//改为手动调节
/*static void BLE_LightSize_Change(unsigned char size)
{
	unsigned short crcval = 0;
	unsigned char reply_buff[9] = {0};
	unsigned char change_size = size;
	
	if(change_size <= 1)//判断参数有效性
	{
		Set_LightSize(change_size);
		reply_buff[5] = 1;//设置成功
	}
	else
	{
		reply_buff[5] = 0;//设置成功
	}
	
	reply_buff[0] = BLE_HEADER;
	reply_buff[1] = RIFLE_SLAVE;//源地址
	reply_buff[2] = 0x00;//目的地址
	reply_buff[3] = BLE_LIGHT_SIZE;//指令码
	reply_buff[4] = 0x01;//数据长度

	crcval = CRC_Check(reply_buff, 6);
	reply_buff[6] = crcval >> 8;
	reply_buff[7] = crcval;
	reply_buff[8] = BLE_TAIL;
	
	BLE_TX(reply_buff, 9);
}*/

//蓝牙模块每次被主机(即显控)更改MAC后，主动上报给STM32改枪号，在这里处理更改枪号
static void BLE_MAC_QueryHandle(unsigned char *buf, unsigned char len)
{
	unsigned char idbuff[3] = {0};
	//若MAC为C1-21-00-00-01-02
	//传过来的顺序也是这样的顺序
	//其中枪号为21-01-02，要倒序后再传给ir.c的处理函数Gun_Change_OnlyID()
	idbuff[0] = buf[10];//02
	idbuff[1] = buf[9];//01
	idbuff[2] = buf[6];//21
	Gun_Change_OnlyID(idbuff);
}

//////////////////////////////////////////////////
//按键回调函数
static void KeyEventHandle(KEY_VALUE_TYPEDEF keys)
{
	static unsigned char ir_key = 0;

	if(Train_Info.Screen_Sta == 1)//熄屏的时候按键值就不传给菜单处理了
	{
		pModeMenu->keyVal = keys;
	}
	
	if(keys == KEY_SET_CLICK_RELEASE)
	{
		if(Train_Info.Train_Sta == Train_OnSta)
		{
			Train_Info.TurnOff_Screen_Cnt = 0;
			if(Train_Info.Screen_Sta == 0)
			{
				pModeMenu = &generalModeMenu[GNL_SHOOT_INFO];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
				ScreenControl(1);//亮屏
			}
		}
		
	}
	else if(keys == KEY_SET_LONG_PRESS)
	{
		//取消按键配对方式，改为APP配对
		/*if(PairKey_PowerON)
		{
			BEEP_Pair();
		}*/
		
	}
	else if(keys == KEY_SET_LONG_PRESS_RELEASE)
	{
		//取消按键配对方式，改为APP配对
		/*if(PairKey_PowerON)
		{
			PairKey_PowerON = 0;
			unsigned char init_id[2] = {0};
			BLE_Mac_Change(init_id);
		}*/
	}
	else if(keys == KEY_POWER_CLICK_RELEASE)
	{
		if(!ir_key)
		{
			ir_key = 1;
		}
		else
		{
			//上电后如果还处于测试模式，且子弹打完被锁枪
			//那么单击电源键可以复位设备
			if((Query_GunMode() == Dev_TestMode) && (Query_GunLock() == Locked_Gun) && (Query_BulletNum() == 0))
			{
				Reset_Dev();
			}
			else if((Query_GunMode() == Dev_TestMode) && (Query_GunLock() == Unlocked_Gun) && (Query_BulletNum() != 0))
			{
				Ir_Shot();
			}

		}

	}
	else if(keys == KEY_POWER_LONG_PRESS_RELEASE)
	{
		delay_ms(100);
		Machine_LockGun();//机械锁枪
		//关闭显示屏
		OLED_Clear();
		OLED_DisPlay_Off();
		START_KEY = 0;//关机
	}
	else if(keys == KEY_POWER_LONG_PRESS)
	{
		BEEP_PowerOff();
	}
}

//置起配对标志位，允许配对
//改为APP配对
/*void Set_PairKey(void)
{
	PairKey_PowerON = 1;
}

//清除配对允许标志位，不允许配对
void Clear_PairKey(void)
{
	PairKey_PowerON = 0;
}*/

////////////校验函数
//校验和
unsigned char Check_Sum(unsigned char *arr_ptr, unsigned char len) {
    unsigned char sum = 0;
		unsigned char i =0;
		for(;i<len;i++)
		{
			sum += arr_ptr[i];
		}
    return sum;
}

//奇校验
u8 Odd_parity(u8 data)
{
	u8 num = 0;
	u8 i = 0;
	for(;i<8;i++)
	{
		if(((data>>i)&0x01) == 0x01)
		{
			num++;
		}
	}
	if(num%2 == 0)
	{
		return 1;
	}
	else
		return 0;
}

//CRC校验
u16 CRC_Check(u8 *mod_buf,u8 len)
{
	u16 Crc = 0xffff,CRC_swop;
	u8 i = 0,j;
	for(i = 0;i < len;i++)
	{
		Crc=Crc^*(mod_buf+i);
		for(j = 0;j < 8;j++)
		{
			if(Crc&0x1)
			{
				Crc >>= 1;
				Crc = Crc^0xA001;
			}
			else
			{
				Crc >>= 1;
			}
		}
	}
	CRC_swop = (Crc&0xff)<<8;
	CRC_swop |= Crc >> 8&0xff;

	return CRC_swop;
}

