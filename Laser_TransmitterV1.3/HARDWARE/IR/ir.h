#ifndef _IR_H_
#define _IR_H_

#include "sys.h"


//DMA���ݻ���ĳ���
#define GUNID_1BYTE_LEN  	41///1�ֽ�ǹ�Ź�17���ֽڣ�����֡β3�ֽ�0����41���ֽ�
#define GUNID_3BYTE_LEN  	68///3�ֽ�ǹ�Ź�44���ֽڣ�����֡β3�ֽ�0. ��68���ֽ�

//���������λ��DMA���ݻ���ĳ���///����1�ֽ�ǹ�ŵĳ��Ծ��Ǿ��������ز��ĳ��ȣ���1�ֽ�ǹ�Ŵ��ӵ����ģ��Ͳ���У����ˣ��˴���ע�⣬���Ҹ��˵�����
#define GUNID_1BYTE_CNT_LEN  	50///1�ֽ�ǹ�Ź�17���ֽڣ�����֡β3�ֽ�0���Լ�1���ֽڵļ���λ(9bits),��50���ֽ�
#define GUNID_3BYTE_CNT_LEN  	77///3�ֽ�ǹ�Ź�44���ֽڣ�����֡β3�ֽ�0. �Լ�1���ֽڵļ���λ(9bits),��77���ֽ�

//��ͬ����Ƶ���£���������"1"�иߵ�ƽ��Ӧ��cntֵ
#define HIGHBIT_TICK_48K		72
#define HIGHBIT_TICK_200K		60
#define HIGHBIT_TICK_500K		24

//��ͬ����Ƶ���£��������ڶ�Ӧ��cntֵ
#define CYCLE_TICK_48K		500
#define CYCLE_TICK_200K		120
#define CYCLE_TICK_500K		48

//ǹ������
typedef enum{
	GunID_1Byte = 0,
	GUNID_3Byte,
}GunID_Type;

//����λ
typedef enum{
	Cnt_Key_Off = 0,
	Cnt_Key_On,
}Cnt_KeyType;

//����Ƶ��
typedef enum{
	Freq_500K = 0,
	Freq_200K,
	Freq_48K,
}IR_Freq_Set;

//typedef enum{
//	Target_Mode = 0,//���ģʽ
//	Confront_Mode,  //�Կ�ģʽ
//}IR_ModeType;

//��������Ľṹ��
typedef struct{
	unsigned char ID_Type;//ǹ�����ͣ�1�ֽ�ǹ��/3�ֽ�ǹ��
	unsigned char Cnt_Key;//�������أ�Ϊ1ʱǹ�Ŵ�����λ��Ϊ0ʱ������
	unsigned char IR_Freq;//����Ƶ�ʣ�48K��200K��500K
	unsigned char Send_Frame;//PWM֡��
	unsigned char ID_Buff[3];//ǹ��
	//unsigned char IR_Mode;//����ģʽ�� =0 ���ģʽ��=1 �Կ�ģʽ
}IR_INFO_STU;


//ǹ��
typedef enum{
	Locked_Gun = 0,//��ǹ
	Unlocked_Gun,//����ǹ
}Gun_LockType;

//ǹеģʽ
typedef enum{
	Dev_TestMode = 0,//�豸����ģʽ��ֻ��30���ӵ�
	Unlimited_BulletMode,//�����ӵ�ģʽ
	Timed_ShootMode,//��ʱ���ģʽ������ģʽ��ͬ����Ҫ�����ӵ�
	Normal_ShootMode,//��ͨ���ģʽ��������ģʽ�����䷢�ӵ�
	Calibrate_Mode,//У׼ģʽ������Уǹ
}Gun_ModeType;

//ǹе��Ϣ�Ľṹ��
typedef struct{
	unsigned char Gun_Lock;//ǹ����=0 ��ǹ��=1 ����ǹ
	unsigned char Gun_Mode;//ǹеģʽ���豸����/�����ӵ�/��ʱ���/��ͨ���ģʽ
	unsigned short Bullet_Num;//�ӵ�����0--65535
	unsigned short Shoot_Cnt;//��������0--65535
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

