#ifndef _APP_H_
#define _APP_H_

#include "sys.h" 


/////////////BLE 通讯////////////////
///帧头帧尾
#define  BLE_HEADER   0x68
#define  BLE_TAIL			0x16

///地址
#define  BLE_MASTER			0x00
#define  RIFLE_SLAVE    0x21
#define  PISTOL_SLAVE   0x20

//MAC
#define  MAC_HEADER			0xC1
#define  RIFLE_MAC			0x21
#define  PISTOL_MAC			0x20

///CMD
#define  IR_INFO_SET   			0x04//8.5设置激光参数
#define  IR_INFO_QUERY			0x05//8.6查询激光参数
#define  BLE_MAC_CHANGE 		0xD1//更改蓝牙从机模块的MAC地址为C1-XX-00-00-00-00，用于配对
#define  BLE_MAC_QUERY			0xD0//8.19查询蓝牙模块地址指令，在这里用于蓝牙模块每次被主机(即显控)更改MAC后，主动上报给STM32改枪号
#define  BLE_TRAIN_START		0x01//8.2开始训练
#define  BLE_TRAIN_END			0x02//8.3结束训练
#define  BLE_DEV_RESET			0x03//8.4设备复位
#define  BLE_DEV_STAQUERY		0x15//0x0A//8.11查询设备状态
#define  BLE_DEV_INFOSET		0x0B//8.12设置设备状态
#define  BLE_GUN_SWITCH			0x0C//8.13枪械开关
#define  BLE_SHOT_REPROT		0x0D//8.14射弹信息上报
#define  BLE_BULLET_ADD			0x0E//8.15子弹补充
#define  BLE_LIGHT_SIZE			0x11//8.18设置激光光斑大小
#define  BLE_FIND_DEV				0x14//8.21显控查找设备指令
#define  SHEEP_TEST					0xAA//测试E2PROM读写
//----------------------------------

////////////菜单相关声明///////////////

#define PUTOUT_SCREEN_PERIOD			500			//训练状态下亮屏后没任何操作熄屏时间,5秒

typedef enum
{
	SCREEN_CMD_NULL,		//无用命令
	SCREEN_CMD_RESET,		//重置屏显示
	SCREEN_CMD_RECOVER,		//恢复原来显示
	SCREEN_CMD_UPDATE,		//更新原来显示
}SCREEN_CMD;		//刷新屏显示标志



//定义菜单的位置，用于超时退出判断
typedef enum
{
	DESKTOP_MENU_POS,	//桌面
	DEV_INFO_POS,     //设备信息页面
	SHOOT_INFO_POS,   //射击信息
}MENU_POS;

//普通菜单列表
typedef enum
{
	GNL_MENU_DESKTOP,		//桌面
	GNL_DEV_INFO,       //设备信息
	GNL_SHOOT_INFO,			//射击信息
	GNL_MENU_SUM,
}GENERAL_MENU_LIST;			//普通菜单列表

typedef struct MODE_MENU
{
	unsigned char ID;				//菜单唯一ID号
	MENU_POS menuPos;				//当前菜单的位置信息
	unsigned char *pModeType;		//指向当前模式类型
	void (*action)(void);				//当前模式下的响应函数
	SCREEN_CMD refreshScreenCmd;		//刷新屏显示命令
	unsigned char reserved;				//预留，方便参数传递
	unsigned char keyVal;				//按键值,0xFF代表无按键触发
	struct MODE_MENU *pLast;			//指向上一个选项
	struct MODE_MENU *pNext;			//指向下一个选项
	struct MODE_MENU *pParent;			//指向父级菜单
	struct MODE_MENU *pChild;			//指向子级菜单
}stu_mode_menu;
//------------------------------------------------

//训练状态
typedef enum{
	Train_OffSta = 0,//训练结束
	Train_OnSta,//训练开始状态
}Train_SizeType;

//光斑大小
typedef enum{
	Target_Mode = 0,//打靶模式，小光斑
	Confront_Mode,//对抗模式，大光斑
}Light_SizeType;

//训练相关的变量
typedef struct{
	unsigned char Train_Sta;//训练状态，用于判断是否要自动息屏
	unsigned char Light_Size;//光斑大小，用于记录光斑模式
	unsigned char Screen_Sta;//屏幕状态，=0熄屏中，=1亮屏中
	unsigned short Timed_Period;//定时射击模式的定时周期
	unsigned short TurnOff_Screen_Cnt;//熄屏的计数
}TRAIN_INFO_STU;

unsigned char Check_Sum(unsigned char *arr_ptr, unsigned char len);//校验和
u8 Odd_parity(u8 data);//奇校验
u16 CRC_Check(u8 *mod_buf,u8 len);//CRC校验
void APP_Init(void);
void App_Proc(void);
void Set_ClrScreenFlag(void);
void Reset_ClrScreenFlag(void);
unsigned char Query_ClrScreenFlag(void);
unsigned char Query_MenuPosition(void);
void Set_MenuPosition(unsigned char page);
void Refresh_GunNum(void);
void BLE_DataHandle(unsigned char *dat, int len);
void IR_Proc(void);
void ScreenControl(unsigned char cmd);
void Change_TrainSta(unsigned char cmd);
unsigned char Query_TrainSta(void);
void BLE_Mac_Change(unsigned char *id_buff);
//void Set_PairKey(void);
//void Clear_PairKey(void);

#endif

