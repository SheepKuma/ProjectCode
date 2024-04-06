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
ʹ��˵����
һ���ⲿ���ú���Ϊ:
1.��ʼ������Ir_Init();
2.���ⷢ�亯��Ir_Shot()��ÿ����һ�η���һ�μ��⣻
3.���ڸ��ļ����������GunID_ChangeI(unsigned char gunid_type, unsigned char cntkey, unsigned char ir_freq, unsigned char *gunid_buff)���������ú���ע��

������ע����ĵĵط�:
1.e2prom��д����;
2.����Ҫ���ÿ�η��伤����ϱ�����������Ir_Shot()��������Ӵ��ڷ�����ش��롣
*******************************************************************************/


	

static void Ir_Config(unsigned short arr, unsigned short psc);
static void Ir_DMAConfig(DMA_Channel_TypeDef * DMAx_Channely, unsigned long PerAddr, unsigned long MemAddr, unsigned long BuffSize);
static void GunIR_InfoInit(void);
static void GunID_To_IRDMABuff(unsigned char gunid_type, unsigned char cntkey, unsigned char ir_freq, unsigned char *gunid_buff);
static void IR_InfoSet(void);
static void DMA_Buff_CntUpdate(unsigned char cnt);
static void Shot_BLEReport(unsigned short shot_cnt, unsigned short bullet_remian);


//ֱ�ӳ�ʼ��Ϊ������λ��buff�����ǲ�������λ����ı�DMA����ĳ��ȣ��Լ�buff��֡β��λ�ü���
unsigned short IrDMA_1ByteBuff[GUNID_1BYTE_CNT_LEN] = {0};//һ�ֽ�ǹ�Ŷ�Ӧ��DMA����,��������λ
unsigned short IrDMA_3ByteBuff[GUNID_3BYTE_CNT_LEN] = {0};//���ֽ�ǹ�Ŷ�Ӧ��DMA����,��������λ

//Ĭ��16���Ƶ�ǹ������(Ԥ��һ������λ)��������MAC��ַΪC1 21 00 00 00 00ʱ���������ǹ��
unsigned char GunID_Buff[5]={0xff,0x00,0x21,0,0};
//unsigned char GunID_Buff[4]={0x01,0x00,0x80,0};
//unsigned char GunID_Buff[4]={0xff,0xff,0xEF,0};


unsigned char frame_num = 0;//һ�μ��ⷢ���PWM֡��
unsigned char frame_cnt = 0;//֡��������ÿ��һ֡��+1
IR_INFO_STU IR_Info = {0};//��������Ľṹ�����
GUN_INFO_STU Gun_Info = {0};//ǹе�����Ľṹ�����
unsigned char DMA_BuffLen = 0;//���ڷ�����һ֡PWM����������DMAͨ����DMA����Ĵ�С
unsigned char Cnt_Flag = 0;//�����ı�־λ��Ϊ1ʱ��ʾ��������Ҫ�м���λ��Ϊ0ʱ��ʾ����Ҫ�Ӽ���λ
unsigned char IRSendNum = 0;//���ⷢ��ļ���
unsigned char GunLock_Flag = 0;//��ǹ��־λ�����ӵ�Ϊ0ʱ���𣬵ȴ�����ɿ�������ǹ������û�ɰ������ǹ��������ǹ���ܿ۶����


/*******************************************************************************
* Function Name  : void Ir_Init(void)
* Description    : ���ڳ�ʼ����ʱ����PWM��DMA��������ã���main�����е���
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

	IR_InfoSet();//��flash/e2prom�ж���������ز���
	//���ݼ���Ƶ�ʳ�ʼ����ʱ��������
	switch(IR_Info.IR_Freq)
	{
		case Freq_48K:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//�޸�Ƶ��
			break;
		case Freq_500K:
			Ir_Config(CYCLE_TICK_500K-1,3-1);//�޸�Ƶ��
			break;
		case Freq_200K:
			Ir_Config(CYCLE_TICK_200K-1,3-1);//�޸�Ƶ��
			break;
		default:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//�޸�Ƶ��
			break;
	}
	//����ǹ�����ͳ�ʼ��DMA����
	if(IR_Info.ID_Type == GunID_1Byte)
	{
		if(Cnt_Flag)//���ݼ�����־λ����DMA buff�ĳ���
		{
			//IRSendNum = 1;//��1��ʼ����
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
		if(Cnt_Flag)//���ݼ�����־λ����DMA buff�ĳ���
		{
			//IRSendNum = 1;//��1��ʼ����
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_3ByteBuff, GUNID_3BYTE_CNT_LEN);
			DMA_BuffLen = GUNID_3BYTE_CNT_LEN;
		}
		else
		{
			Ir_DMAConfig(DMA1_Channel7, (unsigned long)(&TIM4->CCR3), (unsigned long)IrDMA_3ByteBuff, GUNID_3BYTE_LEN);
			DMA_BuffLen = GUNID_3BYTE_LEN;
		}
	}

	GunIR_InfoInit();//��ʼ��ǹе�뼤�����ز���
	Mode_Change_GunID();
}
///���ַ���(ע�⣺����Ƶ��ʱҪ�ǵø���GunID_To_IRDMABuff()�������ߵ�ƽ��BUFFֵ)��
///����һ��24MHz
///48K�� Ir_Config(500-1,3-1),�ߵ�ƽTICKֵΪ72����ʱΪ��׼��20.83US���ڣ�3US�ߵ�ƽ
///200K: Ir_Config(120-1,3-1),�ߵ�ƽTICKֵΪ60����ʱ����Ϊ5US��2.5US�ߵ�ƽ
///500K��Ir_Config(48-1,3-1),�ߵ�ƽTICKֵΪ24�� ��ʱ����Ϊ2US�� 2US�ߵ�ƽ

///��������8MHz
///48K: Ir_Config(167-1,9-1),�ߵ�ƽTICKֵΪ24����ʱ����Ϊ20.88US��3US�ߵ�ƽ
///200K��Ir_Config(40-1,9-1),�ߵ�ƽTICKֵΪ20����ʱ����Ϊ5US��2.5US�ߵ�ƽ
///500K: Ir_Config(16-1,9-1),�ߵ�ƽTICKֵΪ8����ʱ����Ϊ2US��1US�ߵ�ƽ

///���⣬����PWM������TIM_OCInitStructure.TIM_PulseΪ0���ʴ��ⲿ�жϴ��������伤�⣬֮�������ʱ������Ƶ�����


/*******************************************************************************
* Function Name  : static void Ir_Config(unsigned short arr, unsigned short psc)
* Description    : ���ڳ�ʼ����ʱ����PWM���������
* Input          : unsigned short arr--����
									 unsigned short psc--��ʱ������ʱ�ӵ�Ԥ��Ƶֵ
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
	
   //PB8����
	GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
	GPIO_Init(GPIOB, &GPIO_InitStructure);
 
   //��ʼ��TIM4
	TIM_TimeBaseStructure.TIM_Period = arr; //��������һ�������¼�װ�����Զ���װ�ؼĴ������ڵ�ֵ
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //����������ΪTIMxʱ��Ƶ�ʳ�����Ԥ��Ƶֵ
	TIM_TimeBaseStructure.TIM_ClockDivision = 0; //����ʱ�ӷָ�:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM���ϼ���ģʽ
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //����TIM_TimeBaseInitStruct��ָ���Ĳ�����ʼ��TIMx��ʱ�������λ
	
	//��ʼ��TIM4 Channel3 PWMģʽ	 
	TIM_OCInitStructure.TIM_OCMode = TIM_OCMode_PWM1; //ѡ��ʱ��ģʽ:TIM�����ȵ���ģʽ1
 	TIM_OCInitStructure.TIM_OutputState = TIM_OutputState_Enable; //�Ƚ����ʹ��
	TIM_OCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High; //�������:TIM����Ƚϼ��Ը�
	TIM_OCInitStructure.TIM_Pulse = 0;//0;//IrDMA_Buff[0]
	TIM_OC3Init(TIM4, &TIM_OCInitStructure);  //����Tָ���Ĳ�����ʼ������TIM4 OC3
	TIM_OC3PreloadConfig(TIM4, TIM_OCPreload_Enable);  //ʹ��TIM4��CCR3�ϵ�Ԥװ�ؼĴ���
	//TIM_ARRPreloadConfig(TIM4, ENABLE);
	//TIM_GenerateEvent(TIM4, TIM_EventSource_Update);
	TIM_DMACmd(TIM4, TIM_DMA_Update, ENABLE);
	//TIM_Cmd(TIM4, ENABLE);  //ʹ��TIM4
	//TIM_Cmd(TIM4, DISABLE);
	
}

/*******************************************************************************
* Function Name  : static void Ir_DMAConfig(DMA_Channel_TypeDef * DMAx_Channely, unsigned long PerAddr, unsigned long MemAddr, unsigned long BuffSize)
* Description    : ��������DMA��ز������ж�����
* Input          : DMA_Channel_TypeDef * DMAx_Channely--���õ���DMAͨ��
									 unsigned long PerAddr--DMA���ݴ����Ŀ���ַ������ʱ����CCR�Ĵ���
									 unsigned long MemAddr--DMA���ݻ����ַ
									 unsigned long BuffSize--DMA���ݻ�������С
* Output         : None
* Return         : None
* Attention		 	 : None
*******************************************************************************/
static void Ir_DMAConfig(DMA_Channel_TypeDef * DMAx_Channely, unsigned long PerAddr, unsigned long MemAddr, unsigned long BuffSize)
{
	DMA_InitTypeDef DMA_InitStructure;
	RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);//����DMAʱ��
 
	DMA_DeInit(DMAx_Channely);
	while ( 0 != DMA_GetCurrDataCounter(DMAx_Channely)){}//�ȴ�DMA������
	
	//���ø���PB8 PWM���͵�DMA    
	DMA_InitStructure.DMA_PeripheralBaseAddr = PerAddr;			//���ݴ���Ŀ���ַ
	DMA_InitStructure.DMA_MemoryBaseAddr = MemAddr;			//���ݻ����ַ
	DMA_InitStructure.DMA_DIR = DMA_DIR_PeripheralDST;									//������Ϊ���ݴ����Ŀ�ĵ�
	DMA_InitStructure.DMA_BufferSize = BuffSize;						//����Buff���ݴ�С
	DMA_InitStructure.DMA_PeripheralInc = DMA_PeripheralInc_Disable;	 //���������ַ�Ƿ����
	DMA_InitStructure.DMA_MemoryInc = DMA_MemoryInc_Enable;				 //�����ڴ��ַ�Ƿ����
	DMA_InitStructure.DMA_PeripheralDataSize = DMA_PeripheralDataSize_HalfWord;	//�������ݿ��Ϊ8λ
	DMA_InitStructure.DMA_MemoryDataSize = DMA_MemoryDataSize_HalfWord;		 	//�ڴ����ݿ��Ϊ8λ
	DMA_InitStructure.DMA_Mode = DMA_Mode_Normal;		  	//��ͨ����ģʽ
	DMA_InitStructure.DMA_Priority = DMA_Priority_VeryHigh;	   //������ȼ�
	DMA_InitStructure.DMA_M2M = DMA_M2M_Disable;	  		//��ֹDMA2���ڴ��໥����
	DMA_Init(DMAx_Channely, &DMA_InitStructure);		  	//��ʼ��DMA
}

//��ʼ��ǹе�뼤�����ز���
static void GunIR_InfoInit(void)
{
	IRSendNum = 1;//��1��ʼ����
	frame_cnt = 0;//֡������ʼ��
	Gun_Info.Gun_Lock = Unlocked_Gun;//�ϵ粻��ǹ���Ƚ�������豸ģʽ����30���ӵ�
	Gun_Info.Gun_Mode = Dev_TestMode;//�ϵ粻��ǹ���Ƚ�������豸ģʽ����30���ӵ�
	Gun_Info.Bullet_Num = 30;//�ϵ粻��ǹ���Ƚ�������豸ģʽ����30���ӵ�
	Gun_Info.Shoot_Cnt = 0;//��1��ʼ����
	GunLock_Flag = 0;
}



/*******************************************************************************
* Function Name  : void Ir_Shot(void)
* Description    : ���ⷢ�亯�������ⲿ���ã�ִ��һ�η���һ�μ���
* Input          : None
* Output         : None
* Return         : None
* Attention		 	 : None
*******************************************************************************/
void Ir_Shot(void)
{
	if(Gun_Info.Gun_Lock == Unlocked_Gun)//û��ǹ״̬�²ſ��Է��伤��
	{
		do
		{
			TIM4->CR1 |= TIM_CR1_CEN;//�򿪶�ʱ��
			DMA1_Channel7->CCR &= (uint16_t)(~DMA_CCR1_EN);//�ر�DMAͨ��
			DMA1_Channel7->CNDTR = DMA_BuffLen;//����DMAͨ����DMA����Ĵ�С
			DMA1_Channel7->CCR |= DMA_CCR1_EN;//��DMAͨ��

			while (DMA_GetCurrDataCounter(DMA1_Channel7));
			frame_cnt++;
		}while(frame_cnt<frame_num);
		TIM_Cmd(TIM4, DISABLE);
		DMA1_Channel7->CCR &= (uint16_t)(~DMA_CCR1_EN);//�ر�DMAͨ��
		frame_cnt=0;
		
		//if((Gun_Info.Gun_Mode == Dev_TestMode) || (Gun_Info.Gun_Mode == Normal_ShootMode))//��������ͨ���ģʽ�Ͳ����豸ģʽ�£������ӵ���������������ϱ�����
		if((Gun_Info.Gun_Mode != Unlimited_BulletMode) && (Gun_Info.Gun_Mode != Calibrate_Mode))//ֻҪ���������ӵ�ģʽ��У׼ģʽ���ʹ����ӵ���
		{
			//����������
			if(Gun_Info.Shoot_Cnt == 65535)
			{
				Gun_Info.Shoot_Cnt = 0;
			}
			else
			{
				Gun_Info.Shoot_Cnt++;
			}
			//�ӵ�������
			Gun_Info.Bullet_Num--;
			if(Gun_Info.Bullet_Num == 0)
			{
				Gun_Info.Gun_Lock = Locked_Gun;//��ǹ
				GunLock_Flag = 1;
				//Machine_LockGun();//��ǹ
				if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//���²˵�ҳ��
				{
					Set_ClrScreenFlag();
				}
			}
			Shot_BLEReport(Gun_Info.Shoot_Cnt, Gun_Info.Bullet_Num);//�ϱ�����
		}
		//�����������ӵ�ģʽ�£���ֻ������������������ӵ�����Ȼ���ϱ�����
		else if(Gun_Info.Gun_Mode == Unlimited_BulletMode)
		{
			//����������
			if(Gun_Info.Shoot_Cnt == 65535)
			{
				Gun_Info.Shoot_Cnt = 0;
			}
			else
			{
				Gun_Info.Shoot_Cnt++;
			}
			Shot_BLEReport(0xffff, 0xffff);//�ϱ���������0xffff�Ǹ���Э�����ģ�Ҳ���԰�Gun_Info.Shoot_Cnt����ȥ
		}
		else if(Gun_Info.Gun_Mode == Calibrate_Mode)//������У׼ģʽ�£����������ӵ�����������ֻ�ϱ�����
		{
			Shot_BLEReport(0xffff, 0xffff);
		}
		
		//�����м���λ����
		if(IRSendNum == 0xFF)
		{
			IRSendNum = 1;//��1��ʼ����
		}
		else
		{
			IRSendNum++;//���ⷢ�������ÿ����һ�μ����+1
		}
		if(Cnt_Flag)//��ʹ���˼�����־λ����ÿ�η����꼤�⣬�������´δ����͵�DMA buff
		{
			DMA_Buff_CntUpdate(IRSendNum);//�������µļ���ֵ�������´δ����͵�DMA buff
		}
	}
		
}


/*******************************************************************************
* Function Name  : static void IR_InfoSet(void)
* Description    : �������ü�����ز���(����Ƶ�ʡ�ǹ��λ����PWM֡��)����Ir_Init()��ʼ��ʱ����
* Input          : None
* Output         : None
* Return         : None
* Attention		 	 : ������flash��û�б�����ز���ʱ������Ĭ�����ó�48KƵ�ʡ�3�ֽ�ǹ�š�3֡PWM��ǹ��68-02-19
*******************************************************************************/
static void IR_InfoSet(void)
{
	unsigned char ff_buff[3] = {0xff,0xff,0xff};
	///TODO:�����E2PROM��д
	I2C_Read(0, (unsigned char *)&IR_Info, sizeof(IR_Info));
	//STMFLASH_ReadForByte(IRINFO_SAVE_ADDR, (u8*)&IR_Info, sizeof(IR_Info));
	if((IR_Info.ID_Type != 0xff) && (IR_Info.IR_Freq != 0xff) && (IR_Info.Send_Frame != 0xff) && (IR_Info.Cnt_Key != 0xff) && (memcmp(IR_Info.ID_Buff, ff_buff, 3) != 0))
	{
		memset(GunID_Buff, 0, 5);///�����һ��GunID_Buff����ֹ֮ǰ�ļ���λӰ�쵽У��͵ļ���
		memcpy(GunID_Buff,IR_Info.ID_Buff,3);
		if(GunID_Buff[0] == 0)//��IDΪ0ʱ���Ͱ�ID����ff�������·�������
		{
			GunID_Buff[0] = 0xff;
		}
		Cnt_Flag = IR_Info.Cnt_Key;
		GunID_Buff[4] = Check_Sum(GunID_Buff,3);//���У���
		GunID_To_IRDMABuff(IR_Info.ID_Type, Cnt_Flag, IR_Info.IR_Freq, GunID_Buff);
		frame_num = IR_Info.Send_Frame;
	}
	else
	{  ///Ĭ��48KƵ�ʣ�3�ֽ�ǹ�Ŵ�����λ������֡��Ϊ3֡
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
* Description    : ���ڸı伤����ز���(����Ƶ�ʡ�ǹ��λ����PWM֡��),�������յ���Ӧָ��ʱ���ô˺������ļ������
* Input          : unsigned char gunid_type--ǹ��λ��(1���ֽ�/3���ֽ�)
									 unsigned char cntkey--�������أ�Ϊ1ʱ���������λ��Ϊ0ʱ��������λ
									 unsigned char ir_freq--����Ƶ��(48K/200K/500K)
									 unsigned char sendframe--PWM֡��(1-255)
									 unsigned char *gunid_buff--�����ǹ��
* Output         : None
* Return         : None
* Attention		 	 : None
*******************************************************************************/
void GunID_Change(unsigned char gunid_type, unsigned char cntkey, unsigned char ir_freq, unsigned char sendframe, unsigned char *gunid_buff)
{
	unsigned char type = gunid_type, freq = ir_freq; 
	unsigned char change_mac_flag = 0;//����MAC�ı�־λ�����յ���ǹ�Ų�ȫΪ0ʱ����֪ͨ����ģ�����MAC��ַ
	unsigned char zero_buff[3] = {0x00,0x00,0x00};
	
	IR_Info.Cnt_Key = Cnt_Flag = cntkey;
	IR_Info.Send_Frame = frame_num = sendframe;
	IR_Info.ID_Type = type;
	IR_Info.IR_Freq = freq;

	switch(IR_Info.IR_Freq)
	{
		case Freq_48K:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//�޸�Ƶ��
			break;
		case Freq_500K:
			Ir_Config(CYCLE_TICK_500K-1,3-1);//�޸�Ƶ��
			break;
		case Freq_200K:
			Ir_Config(CYCLE_TICK_200K-1,3-1);//�޸�Ƶ��
			break;
		default:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//�޸�Ƶ��
			break;
	}
	
	if(type == GunID_1Byte)
	{
		if(memcmp(gunid_buff, zero_buff, 3))//���0x04���ü������ָ�������ǹ��ȫ��0����ô�Ͳ�����ǹ��
		{
			memset(GunID_Buff, 0, 5);///�����һ��GunID_Buff����ֹ֮ǰ�ļ���λӰ�쵽У��͵ļ���//��Ȼ1�ֽ�û��У�飬������ǹ��ǰ������һ��
			GunID_Buff[0] = gunid_buff[0];
			change_mac_flag = 1;//��־��Ҫ����MAC��ַ
		}
		//����Ƶ�ʿ��ܸ����ˣ����Ծ���0x04���ü������ָ�������ǹ��ȫ��0��Ҳ������װ��DMA����
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
		if(memcmp(gunid_buff, zero_buff, 3))//���0x04���ü������ָ�������ǹ��ȫ��0����ô�Ͳ�����ǹ��
		{
			memset(GunID_Buff, 0, 5);///�����һ��GunID_Buff����ֹ֮ǰ�ļ���λӰ�쵽У��͵ļ���
			memcpy(GunID_Buff,gunid_buff,3);
			GunID_Buff[4] = Check_Sum(GunID_Buff,3);
			change_mac_flag = 1;//��־��Ҫ����MAC��ַ
		}
		//����Ƶ�ʿ��ܸ����ˣ����Ծ���0x04���ü������ָ�������ǹ��ȫ��0��Ҳ������װ��DMA����
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
	
	//�������MAC��ַ�ı�־λ������,�Ǿ�˵��Ҫ��MAC��ַ��
	if(change_mac_flag)
	{
		BLE_Mac_Change(GunID_Buff);
	}

}

//ֻ����ǹ�ţ������������������
//����ÿ��BLE����(�Կ�)����BLE�ӻ���MAC�󣬴ӻ�����ͨ������֪ͨSTM32�ı�ǹ�ţ���ʱ�Ͳ���Ҫ�ı�����������ֻ��ǹ�ž���
void Gun_Change_OnlyID(unsigned char *gunid_buff)
{
	unsigned char zero_buff[3] = {0x00,0x00,0x00};

	switch(IR_Info.IR_Freq)
	{
		case Freq_48K:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//�޸�Ƶ��
			break;
		case Freq_500K:
			Ir_Config(CYCLE_TICK_500K-1,3-1);//�޸�Ƶ��
			break;
		case Freq_200K:
			Ir_Config(CYCLE_TICK_200K-1,3-1);//�޸�Ƶ��
			break;
		default:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//�޸�Ƶ��
			break;
	}
	
	if(IR_Info.ID_Type == GunID_1Byte)
	{
		if(memcmp(gunid_buff, zero_buff, 3) != 0)//���0x04���ü������ָ�������ǹ��ȫ��0����ô�Ͳ�����ǹ��
		{
			memset(GunID_Buff, 0, 5);///�����һ��GunID_Buff����ֹ֮ǰ�ļ���λӰ�쵽У��͵ļ���//��Ȼ1�ֽ�û��У�飬������ǹ��ǰ������һ��
			GunID_Buff[0] = gunid_buff[0];
		}
		//����Ƶ�ʿ��ܸ����ˣ����Ծ���0x04���ü������ָ�������ǹ��ȫ��0��Ҳ������װ��DMA����
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
		if(memcmp(gunid_buff, zero_buff, 3) != 0)//���0x04���ü������ָ�������ǹ��ȫ��0����ô�Ͳ�����ǹ��
		{
			memset(GunID_Buff, 0, 5);///�����һ��GunID_Buff����ֹ֮ǰ�ļ���λӰ�쵽У��͵ļ���
			memcpy(GunID_Buff,gunid_buff,3);
			GunID_Buff[4] = Check_Sum(GunID_Buff,3);
		}
		//����Ƶ�ʿ��ܸ����ˣ����Ծ���0x04���ü������ָ�������ǹ��ȫ��0��Ҳ������װ��DMA����
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


//�������豸����ģʽ��������ʽ����ģʽ���л����Ⲩ���е�ǹ��
//�豸����ģʽ��ǹ�ŵ�IDΪff������ģʽ��ǹ��Ϊ������ID��
void Mode_Change_GunID(void)
{
	/*switch(IR_Info.IR_Freq)
	{
		case Freq_48K:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//�޸�Ƶ��
			break;
		case Freq_500K:
			Ir_Config(CYCLE_TICK_500K-1,3-1);//�޸�Ƶ��
			break;
		case Freq_200K:
			Ir_Config(CYCLE_TICK_200K-1,3-1);//�޸�Ƶ��
			break;
		default:
			Ir_Config(CYCLE_TICK_48K-1,3-1);//�޸�Ƶ��
			break;
	}*/
	
	if(IR_Info.ID_Type == GunID_1Byte)
	{
		memset(GunID_Buff, 0, 5);///�����һ��GunID_Buff����ֹ֮ǰ�ļ���λӰ�쵽У��͵ļ���//��Ȼ1�ֽ�û��У�飬������ǹ��ǰ������һ��
		memcpy(GunID_Buff,IR_Info.ID_Buff,3);
		if(Gun_Info.Gun_Mode == Dev_TestMode)
		{
			GunID_Buff[0] = 0xff;
		}
		//����װ��DMA����
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
		memset(GunID_Buff, 0, 5);///�����һ��GunID_Buff����ֹ֮ǰ�ļ���λӰ�쵽У��͵ļ���
		memcpy(GunID_Buff,IR_Info.ID_Buff,3);
		if(Gun_Info.Gun_Mode == Dev_TestMode)
		{
			GunID_Buff[0] = 0xff;
		}
		GunID_Buff[4] = Check_Sum(GunID_Buff,3);

		//����װ��DMA����
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
* Description    : ���ݼ���Ĳ�����������ǹ��ת���ɴ������DMA���飬�˺�����GunID_Change()�е���
* Input          : unsigned char gunid_type--ǹ��λ��(1���ֽ�/3���ֽ�)
									 unsigned char cntkey--�������أ������������Ƿ������λ
									 unsigned char ir_freq--����Ƶ��(48K/200K/500K)
									 unsigned char *gunid_buff--�����ǹ��
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
		//Ϊ�˷�ֹBuff�Ӵ�����ת���ɲ�������������ļ���λ��Ӱ��3�ֽ�0
		//��ÿ����������ǰ�Ƚ�buff��գ�����װ��
		memset(IrDMA_1ByteBuff, 0, GUNID_1BYTE_CNT_LEN * 2);//buff��unsigned short���ͣ����ֽ�����Ҫ��2������ŷ��ִ���
		
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
		
		if(cntkey)//�ж��Ƿ���Ҫ�Ӽ���λ
		{
			IrDMA_1ByteBuff[22] = highbit_tick;
			IrDMA_1ByteBuff[23] = highbit_tick;
			IrDMA_1ByteBuff[24] = 0;
			IrDMA_1ByteBuff[25] = highbit_tick;
			
			IRSendNum = 1;//����Ҫ�Ӽ���λ���򽫼����������㣬��1��ʼ����
			DMA_Buff_CntUpdate(1);//����һ�¼���λΪ1ʱ��DMA buff
		}
		else
		{
			IrDMA_1ByteBuff[13] = highbit_tick;
			IrDMA_1ByteBuff[14] = highbit_tick;
			IrDMA_1ByteBuff[15] = 0;
			IrDMA_1ByteBuff[16] = highbit_tick;
		}

	}
	else///3�ֽ�ǹ��
	{
		//Ϊ�˷�ֹBuff�Ӵ�����ת���ɲ�������������ļ���λ��Ӱ��3�ֽ�0
		//��ÿ����������ǰ�Ƚ�buff��գ�����װ��
		memset(IrDMA_3ByteBuff, 0, GUNID_3BYTE_CNT_LEN * 2);//buff��unsigned short���ͣ����ֽ�����Ҫ��2������ŷ��ִ���
		
		IrDMA_3ByteBuff[0] = highbit_tick;
		IrDMA_3ByteBuff[1] = 0;
		IrDMA_3ByteBuff[2] = highbit_tick;
		IrDMA_3ByteBuff[3] = highbit_tick;
		
		if(cntkey)//�ж��Ƿ���Ҫ�Ӽ���λ
		{
			num = 5;
			IRSendNum = 1;//����Ҫ�Ӽ���λ���򽫼����������㣬��1��ʼ����
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
			gunid_buff[3] = gunid_buff[4];//��У��ͷŵ�ǰһλ��������ת����Bit����
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
* Description    : ����ÿ�μ���������´δ����͵�DMA buff, ��ʹ���˼���ʱ���˺�����Ir_Shot()�е���
* Input          : unsigned char cnt--����ֵ
* Output         : None
* Return         : None
* Attention		 	 : None
*******************************************************************************/
static void DMA_Buff_CntUpdate(unsigned char cnt)
{
	unsigned char high_tick = 0;
	unsigned char i = 0, j = 0, bit = 0, cnt_val = cnt;
	
	//���ȸ��ݼ���Ƶ��ȷ����Ҫ���ĸߵ�ƽtickֵ
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
	//Ȼ�����ǹ�����ͣ�ѡ��ͬ��buff����䷽ʽ
	switch(IR_Info.ID_Type)
	{
		case GunID_1Byte://1�ֽ�ǹ��û��У���
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
		
		case GUNID_3Byte://3�ֽ�ǹ����Ҫÿ��ͬʱ����У��͵���ֵ
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

//�䵯���ϱ�BLE
static void Shot_BLEReport(unsigned short shot_cnt, unsigned short bullet_remian)
{
	unsigned short crcval = 0;
	unsigned char report_buff[18] = {0};
	
	report_buff[0] = BLE_HEADER;//֡ͷ
	report_buff[1] = RIFLE_SLAVE;//Դ��ַ
	report_buff[2] = BLE_MASTER;//Դ��ַ
	report_buff[3] = BLE_SHOT_REPROT;//ָ����
	report_buff[4] = 10;//���ݳ���
	
	report_buff[5] = GunID_Buff[0];//MAC
	report_buff[6] = GunID_Buff[1];//MAC
	report_buff[7] = 0;//MAC
	report_buff[8] = 0;//MAC
	report_buff[9] = RIFLE_MAC;//MAC
	report_buff[10] = MAC_HEADER;//MAC
	
	report_buff[11] = bullet_remian >> 8;//ʣ���ӵ���
	report_buff[12] = bullet_remian & 0xff;//ʣ���ӵ���
	report_buff[13] = shot_cnt >> 8;//�ѻ�����
	report_buff[14] = shot_cnt & 0xff;//�ѻ�����
	crcval = CRC_Check(report_buff, 15);
	report_buff[15] = crcval >> 8;
	report_buff[16] = crcval;
	report_buff[17] = BLE_TAIL;
	
	BLE_TX(report_buff, 18);
}

//BLE�ظ���ѯ���������ָ��
void IR_InfoQuery_Reply(void)
{
	unsigned short crcval = 0;
	unsigned char reply_buff[15] = {0};
	
	reply_buff[0] = BLE_HEADER;
	reply_buff[1] = RIFLE_SLAVE;//Դ��ַ
	reply_buff[2] = BLE_MASTER;//Ŀ�ĵص�ַ
	reply_buff[3] = IR_INFO_QUERY;//ָ����
	reply_buff[4] = 0x07;//���ݳ���
	reply_buff[5] = IR_Info.ID_Type;//ǹ��λ��
	reply_buff[6] = IR_Info.Cnt_Key;//��������
	reply_buff[7] = IR_Info.IR_Freq;//����Ƶ��
	reply_buff[8] = IR_Info.Send_Frame;//����֡��
	memcpy(&reply_buff[9], IR_Info.ID_Buff, 3);//ǹ��
	crcval = CRC_Check(reply_buff, 12);
	reply_buff[12] = crcval >> 8;
	reply_buff[13] = crcval;
	reply_buff[14] = BLE_TAIL;
	
	BLE_TX(reply_buff, 15);
}

//BLE�ظ���ѯǹе������ָ��
void Gun_DevInfo_Reply(void)
{
	unsigned short crcval = 0;
	unsigned char reply_buff[17] = {0};
	
	reply_buff[0] = BLE_HEADER;
	reply_buff[1] = RIFLE_SLAVE;//Դ��ַ
	reply_buff[2] = BLE_MASTER;//Ŀ�ĵص�ַ
	reply_buff[3] = BLE_DEV_STAQUERY;//ָ����
	reply_buff[4] = 0x09;//���ݳ���
	reply_buff[5] = Get_Battery();//�����ٷֱ�
	reply_buff[6] = Gun_Info.Gun_Lock;//ǹе��
	reply_buff[7] = Gun_Info.Gun_Mode;//���ģʽ
	reply_buff[8] = Gun_Info.Bullet_Num >> 8;//ʣ���ӵ���
	reply_buff[9] = Gun_Info.Bullet_Num & 0xff;//ʣ���ӵ���
	reply_buff[10] = Gun_Info.Shoot_Cnt >> 8;//������
	reply_buff[11] = Gun_Info.Shoot_Cnt & 0xff;//������
	reply_buff[12] = Query_TimerPeriod(T_TIMED_SHOOT) >> 8;//��ʱƵ��
	reply_buff[13] = Query_TimerPeriod(T_TIMED_SHOOT) & 0xff;//��ʱƵ��
	crcval = CRC_Check(reply_buff, 14);
	reply_buff[14] = crcval >> 8;
	reply_buff[15] = crcval;
	reply_buff[16] = BLE_TAIL;
	
	BLE_TX(reply_buff, 17);
}

//BLE����ǹе����
void GunInfo_Set(unsigned char *buf, unsigned char len)
{
	unsigned short crcval = 0, period_time = 0;
	unsigned char reply_buff[9] = {0};
	unsigned char last_mode = 0xff, new_mode = 0xff;
	
	reply_buff[5] = 0x01;//Ĭ�����óɹ�
	//���ж��ǲ��Ƕ�ʱ���ģʽ������ǣ���ȡ����ʱƵ��
	if(buf[5] == Timed_ShootMode)
	{
		period_time = (buf[8] << 8) + buf[9];
		if(period_time < 100)
		{
			reply_buff[5] = 0x00;//�������С��100ms,������ʧ��
		}
		else
		{
			Change_TimerPeriod(T_TIMED_SHOOT, period_time);//�������ȷ���ã���ֱ�Ӹı������ʱ������
		}
	}
	if(buf[5] == Calibrate_Mode)//�����У׼ģʽ���򴴽������ʱ��
	{
		CreatTimer(T_IR_CALIBRATE, IR_Calibrate_Proc, 200, T_STOP_STA);//��ʱ���ģʽ
	}
	//������Ƕ�ʱ���ģʽ�����ж��Ƿ�����ȷ��ģʽ����
	if((buf[5] <= 4) && (reply_buff[5] == 0x01))
	{
		last_mode = Gun_Info.Gun_Mode;//�����ģʽ
		new_mode = Gun_Info.Gun_Mode = buf[5];//����ǹеģʽ
		Gun_Info.Shoot_Cnt = 0;//��0��ʼ����
		//�ӵ���ÿ800ms���ڼ����£������ﲻȥ����ˢ����
		Gun_Info.Bullet_Num = (buf[6] << 8) + buf[7];
		reply_buff[5] = 0x01;//���óɹ�
		
		//׼����ʼ����������ǹ���ȴ���ʼѵ��ָ���ٽ���
		Gun_Info.Gun_Lock = Locked_Gun;
		Machine_LockGun();//��е��ǹ
		Set_MenuPosition(2);
		if(last_mode != new_mode)//�¾�״̬��һ��������Ҫ����״̬
		{
			if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//���Ŀǰû���豸��Ϣҳ�棬��ô���Բ�����ˢ��
			{
				Set_ClrScreenFlag();
			}
		}
	}
	else
	{
		reply_buff[5] = 0x00;//����ʧ��
	}
	
	if(reply_buff[5] == 0x01)//����ɹ����ã���ô�ͽ�ǹ������Ϊ����ǹ��
	{
		Mode_Change_GunID();
	}
	delay_ms(20);
	//BLE�ظ�
	reply_buff[0] = BLE_HEADER;
	reply_buff[1] = RIFLE_SLAVE;//Դ��ַ
	reply_buff[2] = BLE_MASTER;//Ŀ�ĵص�ַ
	reply_buff[3] = BLE_DEV_INFOSET;//ָ����
	reply_buff[4] = 0x01;//���ݳ���
	crcval = CRC_Check(reply_buff, 6);
	reply_buff[6] = crcval >> 8;
	reply_buff[7] = crcval;
	reply_buff[8] = BLE_TAIL;
	
	BLE_TX(reply_buff, 9);
}

//BLE����ǹе����
void Gun_LockSet(unsigned char key)
{
	unsigned short crcval = 0;
	unsigned char reply_buff[9] = {0};
	unsigned char last_lock = 0xff, new_lock = 0xff;
	
	last_lock = Gun_Info.Gun_Lock;//����һ��֮ǰ����ǹ״̬
	//������ǹ��
	if(key == 0)
	{
		new_lock = Gun_Info.Gun_Lock = Locked_Gun;//������ǹ״̬
		Machine_LockGun();
		reply_buff[5] = 1;//���óɹ�
	}
	else if(key == 1)
	{
		new_lock = Gun_Info.Gun_Lock = Unlocked_Gun;//������ǹ״̬
		Machine_UnlockGun();
		reply_buff[5] = 1;//���óɹ�
	}
	else
	{
		reply_buff[5] = 0;//���ò��ɹ�
	}
	
	if(last_lock != new_lock)//�����ǹ״̬�仯�ˣ��Ǿ�ˢ����Ļ
	{
		if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//���Ŀǰû���豸��Ϣҳ�棬��ô���Բ�����ˢ��
		{
			Set_ClrScreenFlag();
		}
	}
	
	//�ظ�BLE
	reply_buff[0] = BLE_HEADER;
	reply_buff[1] = RIFLE_SLAVE;//Դ��ַ
	reply_buff[2] = BLE_MASTER;//Ŀ�ĵص�ַ
	reply_buff[3] = BLE_GUN_SWITCH;//ָ����
	reply_buff[4] = 0x01;//���ݳ���
	crcval = CRC_Check(reply_buff, 6);
	reply_buff[6] = crcval >> 8;
	reply_buff[7] = crcval;
	reply_buff[8] = BLE_TAIL;
	
	BLE_TX(reply_buff, 9);
}

//BLE����ǹе�ӵ�
void Gun_BulletAdd(unsigned char *buf, unsigned char len)
{
	unsigned short crcval = 0, bullet_last = 0, bullet_add = 0;
	unsigned char reply_buff[9] = {0};
	
	//�����ӵ�
	bullet_add = (buf[5] << 8) + buf[6];
	bullet_last = 65535 - Gun_Info.Bullet_Num;//���㻹�ж����ӵ�������
	if(bullet_add > bullet_last)
	{
		Gun_Info.Bullet_Num = 65535;//���ᳬ��65535������ô�͸��ӵ�����Ϊ65535��
	}
	else
	{
		Gun_Info.Bullet_Num += bullet_add;//���ᳬ���Ļ�����ֱ�Ӹ�������
	}
	
	reply_buff[0] = BLE_HEADER;
	reply_buff[1] = RIFLE_SLAVE;//Դ��ַ
	reply_buff[2] = BLE_MASTER;//Ŀ�ĵص�ַ
	reply_buff[3] = BLE_BULLET_ADD;//ָ����
	reply_buff[4] = 0x01;//���ݳ���
	reply_buff[5] = 0x01;//���óɹ�
	crcval = CRC_Check(reply_buff, 6);
	reply_buff[6] = crcval >> 8;
	reply_buff[7] = crcval;
	reply_buff[8] = BLE_TAIL;
	
	BLE_TX(reply_buff, 9);
}

//���ڲ�ѯǹеģʽ
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

//������ѯ��ǹ״̬
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

//��ѯʣ���ӵ���
unsigned short Query_BulletNum(void)
{
	return Gun_Info.Bullet_Num;
}

//��ѯ������
unsigned short Query_ShootCnt(void)
{
	return Gun_Info.Shoot_Cnt;
}

//��ʱ�ò�������λ�����ϵ�󼤹ⷢ�����
void Reset_IRNum(void)
{
	IRSendNum = 1;//TODO:�˴���Ҫ���Ӳ��θ���,�����δ�����λ����Ҫͬʱ���²���
}

//BLEָ�ʼѵ��
void Start_Train(void)//TODO:��������������ָʾ
{
	//��ǹ
	Gun_Info.Gun_Lock = Unlocked_Gun;
	Machine_UnlockGun();//��е����
	//Gun_Info.Gun_Mode = Normal_ShootMode;//�ı�״̬Ϊ���ģʽ//���ظı�ģʽ��
	if(Gun_Info.Gun_Mode == Calibrate_Mode)
	{
		ResetTimer(T_IR_CALIBRATE, T_START_STA);//����У׼ģʽ
	}
	
	if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//���²˵�ҳ��
	{
		Set_ClrScreenFlag();
	}
	Gun_Info.Shoot_Cnt = 0;//��0��ʼ����
	if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//���²˵�ҳ��
	{
		Set_ClrScreenFlag();
	}
	BEEP_TrainStart();//����������3��
	Change_TrainSta(1);//��ʼѵ��
	ScreenControl(0);//Ϩ����Ļ
}

//BLEָ���豸��λ
void Reset_Dev(void)//TODO:����������Ĳ���
{
	//��λ�ɸտ���ʱ��״̬
	Gun_Info.Gun_Lock = Unlocked_Gun;
	Machine_UnlockGun();//��е����
	if(Gun_Info.Gun_Mode == Calibrate_Mode)
	{
		CtrlTimer(T_IR_CALIBRATE, T_STOP_STA);//������ڴ���У׼ģʽ���Ǿ��ȹر������ʱ��
	}
	Gun_Info.Gun_Mode = Dev_TestMode;//����ģʽ
	if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//���²˵�ҳ��
	{
		Set_ClrScreenFlag();
	}
	Gun_Info.Bullet_Num = 30;//����30���ӵ�
	Gun_Info.Shoot_Cnt = 0;//��0��ʼ����
	if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//���²˵�ҳ��
	{
		Set_ClrScreenFlag();
	}
	BEEP_PowerOn();//�����µ�һ��������һ��
	if(Query_TrainSta() == Train_OnSta)//�������ѵ��״̬�����˳�ѵ��������
	{
		Change_TrainSta(0);//�˳�ѵ��
		ScreenControl(1);//������Ļ
	}
}

//BLEָ�����ѵ��
void Finish_Train(void)//TODO:������������еĲ���
{
	//��ǹ
	Gun_Info.Gun_Lock = Locked_Gun;
	Machine_LockGun();//��е��ǹ
	if(Gun_Info.Gun_Mode == Calibrate_Mode)
	{
		CtrlTimer(T_IR_CALIBRATE, T_STOP_STA);//������ڴ���У׼ģʽ���Ǿ��ȹر������ʱ��
	}
	Gun_Info.Gun_Mode = Dev_TestMode;//��λΪ����ģʽ
	BEEP_TrainEnd();//ѵ����������6��
	Change_TrainSta(0);//�˳�ѵ��
	ScreenControl(1);//������Ļ
	if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//���²˵�ҳ��
	{
		Set_ClrScreenFlag();
	}
	Gun_Info.Bullet_Num = 0;//��տ����ӵ�
	if((Query_MenuPosition() == DEV_INFO_POS) && (!Query_ClrScreenFlag()))//���²˵�ҳ��
	{
		Set_ClrScreenFlag();
	}
	
}


///У׼���ģʽ�Ĵ�����
void IR_Calibrate_Proc(void)
{
	Ir_Shot();
	ResetTimer(T_IR_CALIBRATE,T_START_STA);
}


