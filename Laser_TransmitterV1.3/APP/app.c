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



//unsigned char writeAry[66] = {0},readAry[66] = {0},i=0;///����I2C��дE2PROM
//unsigned char Stepper_Motor[7] = {0};//���Բ��������I2C��д

TRAIN_INFO_STU Train_Info = {0};

stu_mode_menu *pModeMenu;		//ϵͳ��ǰ�˵�
//��ʼ������˵�
stu_mode_menu generalModeMenu[GNL_MENU_SUM] =
{
	{GNL_MENU_DESKTOP, DESKTOP_MENU_POS, "Desktop", gnlMenu_DesktopCBS, SCREEN_CMD_RESET, 0, 0xFF, 0, 0, 0, 0},
	{GNL_DEV_INFO, DEV_INFO_POS, "DevInfo", gnlMenu_DevInfoCBS, SCREEN_CMD_RESET, 0, 0xFF, 0, 0, 0, 0},
	{GNL_SHOOT_INFO, SHOOT_INFO_POS, "ShootInfo", gnlMenu_ShootInfoCBS, SCREEN_CMD_RESET, 0, 0xFF, 0, 0, 0, 0},
};	

//ˢ����Ļ�ı�־λ, =0����Ҫˢ�£�=1��Ҫˢ��
unsigned char ClrScreenFlag = 0;
//��Ե������־λ��Ϊ1ʱ������ԣ�Ϊ0ʱ��������ԣ������ϵ��3S���������
//unsigned char PairKey_PowerON = 0;//��ΪAPP���

void APP_Init(void)
{
	KeyScan_CBSRegister(KeyEventHandle);//�����ص�
	e2prom_Init();//E2PROM

	ADC_PowerON();//�����ɼ�
	//OLED��Ļ��ʼ��
	OLED_Init();
	OLED_ColorTurn(0);//0������ʾ��1 ��ɫ��ʾ
  OLED_DisplayTurn(1);//0������ʾ 1 ��Ļ��ת��ʾ
	menuInit();//�˵���ʼ��
	//��Ϊ�ṹ���ֶ����ڣ��������ȡ���������
	//Stepper_Motor_Init();//�������
	
	AppInfo_Init();
	
	CreatTimer(T_APP, App_Proc, 10, T_START_STA);//10ms��ѯһ�β˵�
	CreatTimer(T_APP_REFRESH_GUNNUM, Refresh_GunNum, 800, T_START_STA);//800ms��ѯһ���Ƿ���Ҫˢ���䵯��
	CreatTimer(T_TIMED_SHOOT, IR_Proc, Train_Info.Timed_Period, T_STOP_STA);//��ʱ���ģʽ
	CreatTimer(T_STEPPER_CLOSE,Stepper_Motor_Close,5000,T_STOP_STA);//�������5���ػ�
}

//��ʼ��APP��صĲ���
static void AppInfo_Init(void)
{
	//unsigned char size_read = 0;
	//�ȶ�ȡ��ߴ�С
	//��Ϊ�ֶ����ڣ��������ȡ������
	/*I2C_Read(10, (unsigned char *)&size_read, 1);//��ߴ�С�����E2PROM�ĵ�ַ10
	if(size_read > 1)
	{
		Train_Info.Light_Size = size_read = Target_Mode;//����ǵ�һ���ϵ磬���ʼ��ΪС���
		I2C_PageWrite(10, (unsigned char *)&size_read, 1);
	}
	else
	{
		Train_Info.Light_Size = size_read;//���ǵ�һ���ϵ磬�򸳸��ṹ�����
	}*/
	Train_Info.Light_Size = Target_Mode;//������
	ClrScreenFlag = 0;//ˢ����Ļ�ı�־λ
	Train_Info.Train_Sta = Train_OffSta;//�ϵ�Ĭ��Ϊ����ѵ��״̬
	Train_Info.Screen_Sta = 1;//����
	Train_Info.Timed_Period = 200;//��ʱ���ģʽ�������ϵ��Ĭ����200ms������
	Train_Info.TurnOff_Screen_Cnt = 0;//��ʼ��Ϊ0
}

///��ʱ���ģʽ�Ĵ�����
void IR_Proc(void)
{
	Ir_Shot();
	ResetTimer(T_TIMED_SHOOT,T_START_STA);
}

//���ù�ߴ�С
//��Ϊ�ֶ�����
/*
static void Set_LightSize(unsigned char size)
{
	if(size != Train_Info.Light_Size)
	{
		Train_Info.Light_Size = size;
		I2C_PageWrite(10, (unsigned char *)&(Train_Info.Light_Size), 1);
		switch(size)//TODO:ȷ����ת��ת�Ƿ�����ȷ�Ĺ�߱���С����
		{
			case Target_Mode:
				Stepper_Motor_Open();
				delay_ms(10);

				M_I2C_WriteByte(0x20, 0x00, 0x21);  //�Ĵ���ʹ��ͨ����
				///0x40--32ϸ�֣�����0x41--32ϸ�֣�����0x00--256ϸ�֣�����0x01--256ϸ�֣�����
			///0xE0--1����������0xE1--1����������0xA0--4ϸ�֣�����0xA1--4ϸ�֣�����
				M_I2C_WriteByte(0x20,0x01,0x01);     //����ϸ������
				//����Ƶ�ʼ�DEMO
				M_I2C_WriteByte(0x20,0x02,0x00);     //����Ƶ��
				M_I2C_WriteByte(0x20,0x03,0x05);     //����Ƶ��
				//���ݲ����0.047����㲽����--180����0X3BD7,10����0x353
				M_I2C_WriteByte(0x20,0x04,0x53);//0xD7     //����
				M_I2C_WriteByte(0x20,0x05,0x03);//0x3B     //����
				M_I2C_WriteByte(0x20, 0x06, 0x4D);     //���е���60%-0x4D,100%-0x7F,140%-0xBF
				M_I2C_WriteByte(0x20, 0x07, 0x00);     //���ֵ���30%-0x26,100-0x7F,0-0x00
				M_I2C_WriteByte(0x20, 0x08, 0x0F);     //�ȴ�15ms���뱣�ֵ���-0x0F,255ms-0xFF
				M_I2C_WriteByte(0x20,0x09,0x01);     //�������롢��ͣ
				ResetTimer(T_STEPPER_CLOSE, T_START_STA);//5s��رղ������
			break;
			case Confront_Mode:
				Stepper_Motor_Open();
				delay_ms(10);

				M_I2C_WriteByte(0x20, 0x00, 0x21);  //�Ĵ���ʹ��ͨ����
				///0x40--32ϸ�֣�����0x41--32ϸ�֣�����0x00--256ϸ�֣�����0x01--256ϸ�֣�����
			///0xE0--1����������0xE1--1����������0xA0--4ϸ�֣�����0xA1--4ϸ�֣�����
				M_I2C_WriteByte(0x20,0x01,0x00);     //����ϸ������
				//����Ƶ�ʼ�DEMO
				M_I2C_WriteByte(0x20,0x02,0x00);     //����Ƶ��
				M_I2C_WriteByte(0x20,0x03,0x05);     //����Ƶ��
				//���ݲ����0.047����㲽����--180����0X3BD7,10����0x353
				M_I2C_WriteByte(0x20,0x04,0x53);//0xD7     //����
				M_I2C_WriteByte(0x20,0x05,0x03);//0x3B     //����
				M_I2C_WriteByte(0x20, 0x06, 0x4D);     //���е���60%-0x4D,100%-0x7F,140%-0xBF
				M_I2C_WriteByte(0x20, 0x07, 0x00);     //���ֵ���30%-0x26,100-0x7F,0-0x00
				M_I2C_WriteByte(0x20, 0x08, 0x0F);     //�ȴ�15ms���뱣�ֵ���-0x0F,255ms-0xFF
				M_I2C_WriteByte(0x20,0x09,0x01);     //�������롢��ͣ
				ResetTimer(T_STEPPER_CLOSE, T_START_STA);//5s��رղ������
			break;
		}
		if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//���²˵�ҳ��
		{
			Set_ClrScreenFlag();
		}
	}
}*/

//TEST
//��Ϊ�ֶ�����
/*static void Query_Light_Size(void)
{
	unsigned char reply[3] = {0};
	reply[0] = 0x68;
	I2C_Read(10, (unsigned char *)&reply[1], 1);//��ߴ�С�����E2PROM�ĵ�ַ10
	reply[2] = 0x16;
	BLE_TX(reply, 3);
}*/


//������Ļ����cmd-0Ϩ����-1����
void ScreenControl(unsigned char cmd)
{
	if(cmd)
	{
		if(Train_Info.Screen_Sta == 0)//Ϩ����
		{
			Train_Info.Screen_Sta = 1;
			OLED_DisPlay_On();//������Ļ
			Train_Info.TurnOff_Screen_Cnt = 0;//��������
		}
	}
	else
	{
		if(Train_Info.Screen_Sta == 1)//������
		{
			Train_Info.Screen_Sta = 0;
			OLED_DisPlay_Off();//Ϩ����Ļ
			Train_Info.TurnOff_Screen_Cnt = 0;//��������
		}
	}
}

//�ı�ѵ��״̬,cmd-0�˳�ѵ����-1����ѵ��
void Change_TrainSta(unsigned char cmd)
{
	if(cmd)
	{
		Train_Info.Train_Sta =Train_OnSta;//����ѵ��״̬
		Train_Info.TurnOff_Screen_Cnt = 0;
	}
	else
	{
		Train_Info.Train_Sta =Train_OffSta;//�˳�ѵ��״̬
		Train_Info.TurnOff_Screen_Cnt = 0;
		//����ѵ��������ֱ����ʾ�ӵ�ҳ��
		pModeMenu = &generalModeMenu[GNL_SHOOT_INFO];
		pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
	}
}
//��ѯѵ��״̬
unsigned char Query_TrainSta(void)
{
	return Train_Info.Train_Sta;
}

void App_Proc(void)
{
	pModeMenu->action();
	//����ѵ����ʼ״̬��������ʱ�ͼ���������5���Ϩ��
	if((Train_Info.Train_Sta == Train_OnSta) && (Train_Info.Screen_Sta  == 1))
	{
		Train_Info.TurnOff_Screen_Cnt ++;
		if(Train_Info.TurnOff_Screen_Cnt > PUTOUT_SCREEN_PERIOD)
		{
			Train_Info.TurnOff_Screen_Cnt = 0;
			ScreenControl(0);//5S�޲���Ϩ��
		}
	}
	ResetTimer(T_APP,T_START_STA);
}

////////////�˵���ش���/////////////////
static void menuInit(void)
{
	pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];	//�����ϵ���ʾ�Ĳ˵�����Ϊ������ʾ
	pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;	//����ˢ�½����־����������ˢ��ȫ����UI
}
//����˵��Ļص�����
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
		//16��16
		//�����⣺����ǹ�����¹ҡ�
//		OLED_ShowChinese(16,0,0x00,16,1);
//		OLED_ShowChinese(32,0,0x01,16,1);
//		OLED_ShowChinese(48,0,0x11,16,1);
//		OLED_ShowChinese(64,0,0x12,16,1);
//		OLED_ShowChinese(80,0,0x02,16,1);
//		OLED_ShowChinese(96,0,0x03,16,1);
		//Sheep: ����Щ�ÿ���
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
		
		//������ʾ��������:55%��
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
		 
		pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
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
		//ģʽ���⣺�����ģʽ��
		///���ڽṹ��ȡ���˵������Ϊ�ֶ����ڣ�����ϼ�ⲻ����ߵ�ģʽ���ʲ˵�����ʾ�����Ϣ��
		/*if(Train_Info.Light_Size == Target_Mode)//���ģʽ
		{
			OLED_ShowString(0,0,(u8 *)"* ",16,1);
			OLED_ShowChinese(32,0,0x0A,16,1);
			OLED_ShowChinese(48,0,0x0B,16,1);
			OLED_ShowChinese(64,0,0x0C,16,1);
			OLED_ShowChinese(80,0,0x0D,16,1);
		}
		else if(Train_Info.Light_Size == Confront_Mode)//�Կ�ģʽ
		{
			OLED_ShowString(0,0,(u8 *)"* ",16,1);
			OLED_ShowChinese(32,0,0x24,16,1);
			OLED_ShowChinese(48,0,0x25,16,1);
			OLED_ShowChinese(64,0,0x0C,16,1);
			OLED_ShowChinese(80,0,0x0D,16,1);
		}*/
		
		//ǹеģʽ����ǹ״̬������ͨ��� - ������
		//ǹеģʽ����ʾ���´�����BLE����ǹе�����Ĵ�����GunInfo_Set()�����н���
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
		//��ǹ״̬����ʾ���´�����BLE������ǹ�Ĵ�����Gun_LockSet()�����н���
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
		 
		pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
		switch(keys)
		{
			case KEY_SET_CLICK_RELEASE:
				pModeMenu = &generalModeMenu[GNL_SHOOT_INFO];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
		}
	}
}

//�����Ϣ�˵�ҳ��
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
		//�ѻ�����ʾ����������65535����
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
		
		//�ӵ�����ʾ�����ӵ���65535����
		Bullet = Query_BulletNum();
		OLED_ShowString(0,16,(u8 *)"* ",16,1);
		OLED_ShowChinese(16,16,0x19,16,1);
		OLED_ShowChinese(32,16,0x1A,16,1);
		OLED_ShowString(48,16,(u8 *)": ",16,1);
		//���������ӵ�ģʽʱ������ʾ���ӵ��� ���ޡ�
		if(Query_GunMode() == Unlimited_BulletMode)
		{
			OLED_ShowChinese(64,16,0x17,16,1);
			OLED_ShowChinese(80,16,0x18,16,1);
		}
		else//����ģʽ����ʾ�����ӵ���
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
		 
		pModeMenu->keyVal = 0xFF;	//�ָ��˵�����ֵ
		switch(keys)
		{
			case KEY_SET_CLICK_RELEASE:
				pModeMenu = &generalModeMenu[GNL_MENU_DESKTOP];
				pModeMenu->refreshScreenCmd = SCREEN_CMD_RESET;
			break;
		}
	}
}

//����ˢ����Ļ�ı�־λ
void Set_ClrScreenFlag(void)
{
	ClrScreenFlag = 1;
}

//���ˢ����Ļ�ı�־λ
void Reset_ClrScreenFlag(void)
{
	ClrScreenFlag = 0;
}

//��ѯˢ����Ļ�ı�־λ�Ƿ�����
unsigned char Query_ClrScreenFlag(void)
{
	return ClrScreenFlag;
}

//��ѯ��ǰ��ʾ�����ĸ��˵�ҳ��
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

//���ò˵�ҳ����ʾ��ĳ��ҳ��
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

//��ѯ�䵯���Ƿ�Ҫˢ��
void Refresh_GunNum(void)
{
	//�ӵ���
	static unsigned char last_num = 0;
	unsigned char current_num = 0;
	
//	if((Query_GunMode() == Dev_TestMode) || (Query_GunMode() == Normal_ShootMode))//�����ڲ����豸ģʽ����ͨ���ģʽʱ�ż���ӵ���
	if((Query_GunMode() != Unlimited_BulletMode) && (Query_GunMode() != Calibrate_Mode))//ֻҪ���������ӵ�ģʽ��У׼ģʽ���ʹ����ӵ���
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
	else if(Query_GunMode() == Unlimited_BulletMode)//�����ӵ�ģʽ�¾�ֻ�жϻ�������У׼ģʽ��ʲô������
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

//////////BLE ���ݴ���/////////////
void BLE_DataHandle(unsigned char *dat, int len)
{
	unsigned char cmd = dat[3];
	
	switch(cmd)
	{
		case IR_INFO_SET://8.5���ü������
			IR_InfoSet_Reply();
			GunID_Change(dat[5], dat[6], dat[7], dat[8], &dat[9]);
		break;
		case IR_INFO_QUERY://8.6��ѯ�������
			IR_InfoQuery_Reply();
		break;
		case BLE_TRAIN_START://8.2��ʼѵ��
			Start_Train();
		break;
		case BLE_TRAIN_END://8.3����ѵ��
			Finish_Train();
		break;
		case BLE_DEV_RESET://8.4�豸��λ
			Reset_Dev();
		break;
		case BLE_DEV_STAQUERY://8.11��ѯ�豸״̬
			Gun_DevInfo_Reply();
		break;
		case BLE_DEV_INFOSET://8.12�����豸״̬
			GunInfo_Set(dat, len);
		break;
		case BLE_GUN_SWITCH://8.13ǹе����
			Gun_LockSet(dat[5]);
		break;
		case BLE_BULLET_ADD://8.15�ӵ�����
			Gun_BulletAdd(dat, len);
		break;
		case BLE_LIGHT_SIZE://8.18���ù�ߴ�С
			//BLE_LightSize_Change(dat[5]);
		break;
		case BLE_MAC_QUERY://�ӻ�BLEģ���MAC��ַ���������ĺ��ͨ��0xD0ָ���֪ͨSTM32�ı�ǹ��
			BLE_MAC_QueryHandle(dat, len);
		break;
		case BLE_FIND_DEV://�Կز����豸���ӻ��յ��󲻻ظ�������������500ms
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
	reply_buff[1] = RIFLE_SLAVE;//Դ��ַ
	reply_buff[2] = BLE_MASTER;//Ŀ�ĵص�ַ
	reply_buff[3] = IR_INFO_SET;//ָ����
	reply_buff[4] = 0x01;//���ݳ���
	reply_buff[5] = 0x01;//���óɹ�
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
	send_buff[1] = 0xFD;//Դ��ַ��������д����Ϊ����ģ�鲻У��
	send_buff[2] = 0xFE;//Ŀ�ĵ�ַ��������д����Ϊ����ģ�鲻У��
	send_buff[3] = BLE_MAC_CHANGE;//ָ����
	send_buff[4] = 0x06;//���ݳ���
	send_buff[5] = 0xC1;//MAC
	send_buff[6] = RIFLE_MAC;//MAC--ѵ������Աʶ��λ
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
//��Ϊ�ֶ�����
/*static void BLE_LightSize_Change(unsigned char size)
{
	unsigned short crcval = 0;
	unsigned char reply_buff[9] = {0};
	unsigned char change_size = size;
	
	if(change_size <= 1)//�жϲ�����Ч��
	{
		Set_LightSize(change_size);
		reply_buff[5] = 1;//���óɹ�
	}
	else
	{
		reply_buff[5] = 0;//���óɹ�
	}
	
	reply_buff[0] = BLE_HEADER;
	reply_buff[1] = RIFLE_SLAVE;//Դ��ַ
	reply_buff[2] = 0x00;//Ŀ�ĵ�ַ
	reply_buff[3] = BLE_LIGHT_SIZE;//ָ����
	reply_buff[4] = 0x01;//���ݳ���

	crcval = CRC_Check(reply_buff, 6);
	reply_buff[6] = crcval >> 8;
	reply_buff[7] = crcval;
	reply_buff[8] = BLE_TAIL;
	
	BLE_TX(reply_buff, 9);
}*/

//����ģ��ÿ�α�����(���Կ�)����MAC�������ϱ���STM32��ǹ�ţ������ﴦ�����ǹ��
static void BLE_MAC_QueryHandle(unsigned char *buf, unsigned char len)
{
	unsigned char idbuff[3] = {0};
	//��MACΪC1-21-00-00-01-02
	//��������˳��Ҳ��������˳��
	//����ǹ��Ϊ21-01-02��Ҫ������ٴ���ir.c�Ĵ�����Gun_Change_OnlyID()
	idbuff[0] = buf[10];//02
	idbuff[1] = buf[9];//01
	idbuff[2] = buf[6];//21
	Gun_Change_OnlyID(idbuff);
}

//////////////////////////////////////////////////
//�����ص�����
static void KeyEventHandle(KEY_VALUE_TYPEDEF keys)
{
	static unsigned char ir_key = 0;

	if(Train_Info.Screen_Sta == 1)//Ϩ����ʱ�򰴼�ֵ�Ͳ������˵�������
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
				ScreenControl(1);//����
			}
		}
		
	}
	else if(keys == KEY_SET_LONG_PRESS)
	{
		//ȡ��������Է�ʽ����ΪAPP���
		/*if(PairKey_PowerON)
		{
			BEEP_Pair();
		}*/
		
	}
	else if(keys == KEY_SET_LONG_PRESS_RELEASE)
	{
		//ȡ��������Է�ʽ����ΪAPP���
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
			//�ϵ����������ڲ���ģʽ�����ӵ����걻��ǹ
			//��ô������Դ�����Ը�λ�豸
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
		Machine_LockGun();//��е��ǹ
		//�ر���ʾ��
		OLED_Clear();
		OLED_DisPlay_Off();
		START_KEY = 0;//�ػ�
	}
	else if(keys == KEY_POWER_LONG_PRESS)
	{
		BEEP_PowerOff();
	}
}

//������Ա�־λ���������
//��ΪAPP���
/*void Set_PairKey(void)
{
	PairKey_PowerON = 1;
}

//�����������־λ�����������
void Clear_PairKey(void)
{
	PairKey_PowerON = 0;
}*/

////////////У�麯��
//У���
unsigned char Check_Sum(unsigned char *arr_ptr, unsigned char len) {
    unsigned char sum = 0;
		unsigned char i =0;
		for(;i<len;i++)
		{
			sum += arr_ptr[i];
		}
    return sum;
}

//��У��
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

//CRCУ��
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

