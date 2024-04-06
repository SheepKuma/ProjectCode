#ifndef _APP_H_
#define _APP_H_

#include "sys.h" 


/////////////BLE ͨѶ////////////////
///֡ͷ֡β
#define  BLE_HEADER   0x68
#define  BLE_TAIL			0x16

///��ַ
#define  BLE_MASTER			0x00
#define  RIFLE_SLAVE    0x21
#define  PISTOL_SLAVE   0x20

//MAC
#define  MAC_HEADER			0xC1
#define  RIFLE_MAC			0x21
#define  PISTOL_MAC			0x20

///CMD
#define  IR_INFO_SET   			0x04//8.5���ü������
#define  IR_INFO_QUERY			0x05//8.6��ѯ�������
#define  BLE_MAC_CHANGE 		0xD1//���������ӻ�ģ���MAC��ַΪC1-XX-00-00-00-00���������
#define  BLE_MAC_QUERY			0xD0//8.19��ѯ����ģ���ַָ���������������ģ��ÿ�α�����(���Կ�)����MAC�������ϱ���STM32��ǹ��
#define  BLE_TRAIN_START		0x01//8.2��ʼѵ��
#define  BLE_TRAIN_END			0x02//8.3����ѵ��
#define  BLE_DEV_RESET			0x03//8.4�豸��λ
#define  BLE_DEV_STAQUERY		0x15//0x0A//8.11��ѯ�豸״̬
#define  BLE_DEV_INFOSET		0x0B//8.12�����豸״̬
#define  BLE_GUN_SWITCH			0x0C//8.13ǹе����
#define  BLE_SHOT_REPROT		0x0D//8.14�䵯��Ϣ�ϱ�
#define  BLE_BULLET_ADD			0x0E//8.15�ӵ�����
#define  BLE_LIGHT_SIZE			0x11//8.18���ü����ߴ�С
#define  BLE_FIND_DEV				0x14//8.21�Կز����豸ָ��
#define  SHEEP_TEST					0xAA//����E2PROM��д
//----------------------------------

////////////�˵��������///////////////

#define PUTOUT_SCREEN_PERIOD			500			//ѵ��״̬��������û�κβ���Ϩ��ʱ��,5��

typedef enum
{
	SCREEN_CMD_NULL,		//��������
	SCREEN_CMD_RESET,		//��������ʾ
	SCREEN_CMD_RECOVER,		//�ָ�ԭ����ʾ
	SCREEN_CMD_UPDATE,		//����ԭ����ʾ
}SCREEN_CMD;		//ˢ������ʾ��־



//����˵���λ�ã����ڳ�ʱ�˳��ж�
typedef enum
{
	DESKTOP_MENU_POS,	//����
	DEV_INFO_POS,     //�豸��Ϣҳ��
	SHOOT_INFO_POS,   //�����Ϣ
}MENU_POS;

//��ͨ�˵��б�
typedef enum
{
	GNL_MENU_DESKTOP,		//����
	GNL_DEV_INFO,       //�豸��Ϣ
	GNL_SHOOT_INFO,			//�����Ϣ
	GNL_MENU_SUM,
}GENERAL_MENU_LIST;			//��ͨ�˵��б�

typedef struct MODE_MENU
{
	unsigned char ID;				//�˵�ΨһID��
	MENU_POS menuPos;				//��ǰ�˵���λ����Ϣ
	unsigned char *pModeType;		//ָ��ǰģʽ����
	void (*action)(void);				//��ǰģʽ�µ���Ӧ����
	SCREEN_CMD refreshScreenCmd;		//ˢ������ʾ����
	unsigned char reserved;				//Ԥ���������������
	unsigned char keyVal;				//����ֵ,0xFF�����ް�������
	struct MODE_MENU *pLast;			//ָ����һ��ѡ��
	struct MODE_MENU *pNext;			//ָ����һ��ѡ��
	struct MODE_MENU *pParent;			//ָ�򸸼��˵�
	struct MODE_MENU *pChild;			//ָ���Ӽ��˵�
}stu_mode_menu;
//------------------------------------------------

//ѵ��״̬
typedef enum{
	Train_OffSta = 0,//ѵ������
	Train_OnSta,//ѵ����ʼ״̬
}Train_SizeType;

//��ߴ�С
typedef enum{
	Target_Mode = 0,//���ģʽ��С���
	Confront_Mode,//�Կ�ģʽ������
}Light_SizeType;

//ѵ����صı���
typedef struct{
	unsigned char Train_Sta;//ѵ��״̬�������ж��Ƿ�Ҫ�Զ�Ϣ��
	unsigned char Light_Size;//��ߴ�С�����ڼ�¼���ģʽ
	unsigned char Screen_Sta;//��Ļ״̬��=0Ϩ���У�=1������
	unsigned short Timed_Period;//��ʱ���ģʽ�Ķ�ʱ����
	unsigned short TurnOff_Screen_Cnt;//Ϩ���ļ���
}TRAIN_INFO_STU;

unsigned char Check_Sum(unsigned char *arr_ptr, unsigned char len);//У���
u8 Odd_parity(u8 data);//��У��
u16 CRC_Check(u8 *mod_buf,u8 len);//CRCУ��
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

