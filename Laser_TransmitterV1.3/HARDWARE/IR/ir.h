#ifndef _IR_H_
#define _IR_H_

#include "sys.h"


//DMA数据缓存的长度
#define GUNID_1BYTE_LEN  	41///1字节枪号共17个字节，加上帧尾3字节0，共41个字节
#define GUNID_3BYTE_LEN  	68///3字节枪号共44个字节，加上帧尾3字节0. 共68个字节

//激光带计数位的DMA数据缓存的长度///由于1字节枪号的初衷就是尽量减短载波的长度，故1字节枪号带子弹数的，就不加校验和了，此处需注意，是我个人的推论
#define GUNID_1BYTE_CNT_LEN  	50///1字节枪号共17个字节，加上帧尾3字节0，以及1个字节的计数位(9bits),共50个字节
#define GUNID_3BYTE_CNT_LEN  	77///3字节枪号共44个字节，加上帧尾3字节0. 以及1个字节的计数位(9bits),共77个字节

//不同激光频率下，二进制数"1"中高电平对应的cnt值
#define HIGHBIT_TICK_48K		72
#define HIGHBIT_TICK_200K		60
#define HIGHBIT_TICK_500K		24

//不同激光频率下，单个周期对应的cnt值
#define CYCLE_TICK_48K		500
#define CYCLE_TICK_200K		120
#define CYCLE_TICK_500K		48

//枪号类型
typedef enum{
	GunID_1Byte = 0,
	GUNID_3Byte,
}GunID_Type;

//计数位
typedef enum{
	Cnt_Key_Off = 0,
	Cnt_Key_On,
}Cnt_KeyType;

//激光频率
typedef enum{
	Freq_500K = 0,
	Freq_200K,
	Freq_48K,
}IR_Freq_Set;

//typedef enum{
//	Target_Mode = 0,//打靶模式
//	Confront_Mode,  //对抗模式
//}IR_ModeType;

//激光参数的结构体
typedef struct{
	unsigned char ID_Type;//枪号类型：1字节枪号/3字节枪号
	unsigned char Cnt_Key;//计数开关，为1时枪号带计数位，为0时不计数
	unsigned char IR_Freq;//激光频率：48K、200K、500K
	unsigned char Send_Frame;//PWM帧数
	unsigned char ID_Buff[3];//枪号
	//unsigned char IR_Mode;//激光模式， =0 打靶模式，=1 对抗模式
}IR_INFO_STU;


//枪锁
typedef enum{
	Locked_Gun = 0,//锁枪
	Unlocked_Gun,//不锁枪
}Gun_LockType;

//枪械模式
typedef enum{
	Dev_TestMode = 0,//设备测试模式，只给30发子弹
	Unlimited_BulletMode,//无限子弹模式
	Timed_ShootMode,//定时射击模式即连发模式，同样需要消耗子弹
	Normal_ShootMode,//普通射击模式，即比赛模式，需配发子弹
	Calibrate_Mode,//校准模式，用来校枪
}Gun_ModeType;

//枪械信息的结构体
typedef struct{
	unsigned char Gun_Lock;//枪锁，=0 锁枪，=1 不锁枪
	unsigned char Gun_Mode;//枪械模式，设备测试/无限子弹/定时射击/普通射击模式
	unsigned short Bullet_Num;//子弹数，0--65535
	unsigned short Shoot_Cnt;//击发数，0--65535
}GUN_INFO_STU;

extern unsigned char GunLock_Flag;

void Ir_Init(void);
void Ir_Shot(void);
void Reset_IRNum(void);
void GunID_Change(unsigned char gunid_type, unsigned char cntkey, unsigned char ir_freq, unsigned char sendframe, unsigned char *gunid_buff);
void IR_InfoQuery_Reply(void);
void Gun_DevInfo_Reply(void);
void GunInfo_Set(unsigned char *buf, unsigned char len);
void Gun_LockSet(unsigned char key);
void Gun_BulletAdd(unsigned char *buf, unsigned char len);
unsigned char Query_GunMode(void);
unsigned char Query_GunLock(void);
unsigned short Query_BulletNum(void);
unsigned short Query_ShootCnt(void);
void Finish_Train(void);
void Start_Train(void);
void Reset_Dev(void);
void IR_Calibrate_Proc(void);
void Gun_Change_OnlyID(unsigned char *gunid_buff);
void Mode_Change_GunID(void);











#endif

