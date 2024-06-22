

#ifndef __CAN_H
#define __CAN_H

//*** <<< Use Configuration Wizard in Context Menu >>> ***
//This segment should not be modified
//
#ifdef CAN_EXTERN
#define EXTERN

#else

#define EXTERN					extern
#endif

/**********************************************************************
* INFY power module software segment.
***********************************************************************/
////
typedef unsigned char uint8_t;

typedef unsigned short uint16_t;

typedef unsigned int uint32_t;

typedef unsigned long long uint64_t;

//
///////////////////////////////////////////////////////////////////////////////////////////////////
// 锟斤拷锟斤拷��拷缂�拷�?///////////////////////////////////////////////////////////////////////////////////////////////////
//8��拷锟?
typedef struct 
{
unsigned char	Byte0			: 8;
unsigned char	Byte1			: 8;
unsigned char	Byte2			: 8;
unsigned char	Byte3			: 8;
unsigned char	Byte4			: 8;
unsigned char	Byte5			: 8;
unsigned char	Byte6			: 8;
unsigned char	Byte7			: 8;
} Long8Byte;


typedef union 
{
uint64_t		all;
Long8Byte		Byte;
unsigned char	ByteArray[8];
} EightBytes;


//��拷锟界�锟斤�?
typedef struct 
{
unsigned char	bit0			: 1;
unsigned char	bit1			: 1;
unsigned char	bit2			: 1;
unsigned char	bit3			: 1;
unsigned char	bit4			: 1;
unsigned char	bit5			: 1;
unsigned char	bit6			: 1;
unsigned char	bit7			: 1;
} ByteBits;


typedef union 
{
unsigned char	all;
ByteBits		bit;
} ByteBit;


//锟藉�锟芥担锟斤拷娑�拷
typedef struct 
{
unsigned char	bit0			: 1;
unsigned char	bit1			: 1;
unsigned char	bit2			: 1;
unsigned char	bit3			: 1;
unsigned char	bit4			: 1;
unsigned char	bit5			: 1;
unsigned char	bit6			: 1;
unsigned char	bit7			: 1;
unsigned char	bit8			: 1;
unsigned char	bit9			: 1;
unsigned char	bit10			: 1;
unsigned char	bit11			: 1;
unsigned char	bit12			: 1;
unsigned char	bit13			: 1;
unsigned char	bit14			: 1;
unsigned char	bit15			: 1;
unsigned char	bit16			: 1;
unsigned char	bit17			: 1;
unsigned char	bit18			: 1;
unsigned char	bit19			: 1;
unsigned char	bit20			: 1;
unsigned char	bit21			: 1;
unsigned char	bit22			: 1;
unsigned char	bit23			: 1;
unsigned char	bit24			: 1;
unsigned char	bit25			: 1;
unsigned char	bit26			: 1;
unsigned char	bit27			: 1;
unsigned char	bit28			: 1;
unsigned char	bit29			: 1;
unsigned char	bit30			: 1;
unsigned char	bit31			: 1;
} LongWordBits;


typedef union 
{
unsigned long	all;
LongWordBits	bit;
} LongWordBit;


//锟藉�锟斤拷锟斤拷娑�拷
typedef struct 
{
unsigned char	Byte0			: 8;
unsigned char	Byte1			: 8;
} WordBytes;


//4 ��拷锟界�锟斤�?
typedef union 
{
unsigned short	all;
WordBytes		Byte;
unsigned char	Bytes[2];
} WordByte;


//锟藉�锟斤拷锟斤拷娑�拷
typedef struct 
{
unsigned char	Byte0			: 8;
unsigned char	Byte1			: 8;
unsigned char	Byte2			: 8;
unsigned char	Byte3			: 8;
} LongWordBytes;


//4 ��拷锟界�锟斤�?
typedef union 
{
unsigned long	all;
LongWordBytes	Byte;
uint8_t 		Array[4];
} LongWordByte;


//锟斤拷绨挎�锟借�锟斤�?
typedef struct 
{
unsigned char	Byte0			: 8;
unsigned char	Byte1			: 8;
unsigned char	Byte2			: 8;
unsigned char	Byte3			: 8;
} SingleFloatBytes;


//4 ��拷锟界�锟斤�?
typedef union 
{
float			all;
SingleFloatBytes Byte;
} SingleFloatByte;


/**********************************************************************
�������Ͷ���
***********************************************************************/
//�������ID����
typedef struct 
{
unsigned char	ModuleAddr		: 7;				//Դ��ַ
unsigned long	FrameId 		: 22;				//Ŀ���ַ
unsigned char	Rsvd			: 3;
} IncreaseModuleBit;


typedef union 
{
uint32_t		all;
IncreaseModuleBit bit;
} IncreaseModuleID;


typedef union 
{
uint32_t		all;
LongWordByte	VoltOut;
} SetIncreaseModuleVoltOut; //����ģ���ѹ


//���õ�������
typedef union 
{
uint32_t		all;
LongWordByte	CurOut;
} SetIncreaseModuleCurOut; //����ģ�����


////////////////////////////BMS////////////////////
typedef union 
{
uint16_t		all;
WordByte		VoltOut;
} BmsVolt; //����ģ���ѹ


//���õ�������
typedef union 
{
uint16_t		all;
WordByte		CurOut;
} BmsCur; //����ģ�����


//
typedef struct 
{
WordByte		BMSCur; 							//ģ���������
WordByte		BMSVolt;							//ģ�������ѹ

//
WordByte		Bms_temp;							//״̬���
uint8_t 		SOC;
uint8_t 		SOH;
WordByte		BMSVolt_chager_OV;
WordByte		BMSVolt_dischge_UV;
WordByte		BMSCur_chager_max;
WordByte		BMSCur_dischge_max;
WordByte		BMS_CELL_MAX;
WordByte		BMS_CELL_MIN;
WordByte		BMS_CELL_MAX_add;
WordByte		BMS_CELL_MIN_add;
WordByte		BMS_TEMP_MAX;
WordByte		BMS_TEMP_MIN;
WordByte		BMS_TEMP_MAX_add;
WordByte		BMS_TEMP_MIN_add;
uint8_t 		BMS_Sta;
WordByte		cyle_mun;
uint8_t 		Error;
WordByte		Alarm;
WordByte		Protection;
WordByte		RON;

//
WordByte		BMS_ModuleCELL_MAX;
WordByte		BMS_ModuleCELL_MIN;
WordByte		BMS_ModuleCELL_MAX_add;
WordByte		BMS_ModuleCELL_MIN_add;
WordByte		BMS_ModuleTEMP_MAX;
WordByte		BMS_ModuleTEMP_MIN;
WordByte		BMS_ModuleTEMP_MAX_add;
WordByte		BMS_ModuleTEMP_MIN_add;

//
uint8_t 		Nochager_mark;
uint8_t 		Nodischag_mark;
uint8_t 		System_error;
uint8_t 		System_error2;
uint8_t 		System_error3;
uint8_t 		System_almrm2;
uint8_t 		Cell_show;

uint16_t		System_downpower;
} PowerBmsInfo;


//
typedef struct 
{
WordByte		BMSCell_Volt[256];					//
WordByte		BMSCell_Temp[256];					//

} Bms_CellInfo;


//

/**********************************************************************
* INFY power module software segment.
***********************************************************************/
//Ӣ��ԴID����
#define MonitoringAdress		0xF0	  //�ϼ���صĵ�ַ
#define BroadcastAddress		0x3F	  //�㲥��ַ


typedef union 
{
unsigned long	all;


struct 
{
unsigned long	SourceAddr		: 8;				//Դ��ַ
unsigned long	TargetAddr		: 8;				//Ŀ���ַ
unsigned long	ComdCode		: 6;				//�����
unsigned long	EquipmentCode	: 4;				//�豸��
unsigned long	ErrorCode		: 3;				//������
unsigned long	Rsvd			: 3;
} bit;


} INFYModuleID;


//�����룺��ʾ������Ϣ����ԭ��
enum 
{
Normal = 0, 										//����
ComdCodeAbnormal = 2,								//������쳣
DataInformationAbnormal = 3,						//������Ϣ�쳣
AdressInvalid = 4,									//��ַ��Ч
StartupProcedure = 7								//��������
};


//�豸��: ����ȷ��Э��֮����豸����:
enum 
{
SingleModuleProtocol = 0x0A,						//����뵥������ģ��֮��Э��
WholeGroupModuleProtocol = 0x0B 					//�������������ģ��֮��Э��
};


enum 
{
INFYModule_Read = 0,								//��Ӣ��Դģ��
INFYModule_Cofg 									//����Ӣ��Դģ��
};


//�����б�
enum 
{
ReadSysOutPut_Flaot_INFYComd = 0x01,				//��ϵͳ��� ������
ReadSysModuleNum_INFYComd = 0x02,					//��ģ����
ReadSingleOutPut_Flaot_INFYComd = 0x03, 			//��ģ��N��� ������
ReadModuleStatus_INFYComd = 0x04,					//��ȡģ��״̬
ReadSingleInPut_INFYComd = 0x06,					//��ȡģ������
ReadSysOutPut_INFYComd = 0x08,						//��ȡϵͳ��ѹ����
ReadSingleModuleOutPut_INFYComd = 0x09, 			//��ȡ��ģ���ѹ����
ReadSingleModuleInformation_INFYComd = 0x0A,		//��ȡģ��N��Ϣ
CofgModuleWalk_INFYComd = 0x13, 					//����ģ��NWALKʹ�ܽ�ֹ
CofgModuleLed_INFYComd = 0x14,						//����ģ��ƿ�
CofgModuleSysNum_INFYComd = 0x16,					//����ģ��N���
CofgSingleModuleHibernate_INFYComd = 0x19,			//���õ�ģ������
CofgWholeGroupModuleOnOff_INFYComd = 0x1A,			//��������ģ�鿪�ػ�
CofgWholeGroupModuleOutPut_INFYComd = 0x1B, 		//��������ģ�����
CofgSingleModuleOutPut_INFYComd = 0x1C, 			//���õ�ģ�����
CofgModuleReset_INFYComd = 0x1E,					//���õ�ģ�鸴λ
CofgModuleAdress_INFYComdTypeDistribute = 0x1F, 	//����ģ���ַ���䷽ʽ
};


//�궨��
//ģ�鿪�عؼ���
enum 
{
INFYDCModuleOn = 0, 
INFYDCModuleOff = 1
};


//Ӣ��Դ��Դģ����ƹ���
enum 
{
PeriodConfigINFYModule = 0, 						//��������ģ�����
PeriodReadINFYModuleBack							//���ڻض�ģ�����
};


/**********************************************************************
* Power module share internal RAM.
***********************************************************************/
//���锟界�锟斤�?
enum 
{
IncreasePowerModule = 0, 
MegmeetPowerModule, 
HuaWeiPowerModule, 
GaoSiBaoPowerModule, 
TongHePowerModule, 
INFYPowerModule, 
RsvdPowerModule
};


//妤癸拷锟界猾宕�拷ID缁��锟?
typedef struct 
{
unsigned char	ModuleAddr		: 7;				//濠э拷锟斤拷锟?
unsigned long	FrameId 		: 22;				//锟斤拷锟斤拷�匡�?
unsigned char	Rsvd			: 3;
} MegmeetModuleBit;


//妤癸拷锟界猾宕�拷锟借�锟界�锟�缁���?
typedef struct 
{
unsigned char	RES2			: 1;				//妫帮拷锟芥担锟斤拷锟斤拷�舵�锟?
unsigned char	RES1			: 1;				//妫帮拷锟芥担锟斤拷锟斤拷�舵�锟?
unsigned char	CNT 			: 1;				//0��拷�����拷锟斤拷锟斤拷锟斤拷1��拷������锟斤拷锟斤拷锟?
unsigned char	SRCADDR 		: 8;				//濠э拷锟斤拷锟?
unsigned char	DSTADDR 		: 8;				//锟斤拷锟斤拷�匡�?
unsigned char	PTP 			: 1;				//锟界�锟斤拷骞匡�?��拷锟斤拷锟斤拷�靛�锟介�锟介�锟斤拷�靛��?
unsigned short	PROTNO			: 9;				//锟斤拷锟斤拷锟�otocol No��拷x000-0x1ff��拷锟斤拷锟斤拷椋�拷锟姐����锟斤拷缁狙�拷��拷��拷锟斤拷锟斤拷锟�?
unsigned char	Rsvd			: 3;
} MegmeetModuleExtraIDBit;


typedef union 
{
uint32_t		all;
MegmeetModuleBit bit;
} MegmeetModuleID;


typedef union 
{
uint32_t		all;
MegmeetModuleExtraIDBit bit;
} MegmeetModuleExtraID;


//妤癸拷锟界猾宕�拷锟借�锟界�锟�缁���?//锟界�锟界猾璇诧拷锟斤拷�?//锟斤拷锟�D锟斤拷锟?
typedef union 
{
IncreaseModuleID IncreaseID;
MegmeetModuleID MegmeetID;
MegmeetModuleExtraID MegmeetExtraID;				//锟借�锟界�锟?
INFYModuleID	INFYID;
} PowerModuleID;


//�����拷��拷缁���?
typedef union 
{
uint32_t		all;
LongWordByte	CurOut;
} SetMegmeetModuleCurOut; //�����Ο�筹拷锟藉�锟?


//�����拷��拷缁���?
typedef union 
{
uint32_t		all;
LongWordByte	VoltOut;
} SetMegmeetModuleVoltOut; //�����Ο�筹拷锟藉�锟?


//�����拷��拷缁���?
typedef union 
{
SetIncreaseModuleCurOut IncreaseCurOut;
SetMegmeetModuleCurOut MegmeetCurOut;
} SetPowerModuleCurOut; //�����Ο�筹拷锟藉�锟?


//�����拷��拷缁���?
typedef union 
{
SetIncreaseModuleVoltOut IncreaseVoltOut;
SetMegmeetModuleVoltOut MegmeetVoltOut;
} SetPowerModuleVoltOut;


///���Ͻṹ����
typedef struct 
{
unsigned long	ModuleAct		: 1;				//1��ģ��ػ���0��ģ�鿪��
unsigned long	ModuleState 	: 1;				//1��ģ����ϣ�0��ģ������
unsigned long	OutMode 		: 1;				//1: ��������ѹ
unsigned long	FanState		: 1;				//1�����ȹ��ϣ�0����������
unsigned long	InOverVolt		: 1;				//1: �����ѹ��0����������
unsigned long	InUnderVolt 	: 1;				//1: ����Ƿѹ��0����������
unsigned long	OutOverVolt 	: 1;				//1: �����ѹ��0��0����������
unsigned long	OutUnderVolt	: 1;				//1: ���Ƿѹ����������
unsigned long	OverCurrentProtect: 1;				//1������������0������
unsigned long	OverTemperatureProtect: 1;			//1�����±�����0������
unsigned long	SetSwitchModule : 1;				//1: ���ÿ�����0�����ùػ�
unsigned long	PFCOverVolt 	: 1;				//1: PFC��ѹ��0������
unsigned long	PFCUnderVolt	: 1;				//1: PFCǷѹ��0������
unsigned long	ShortProtect	: 1;				//1: ��·������0������
unsigned long	Other			: 15;
unsigned long	QualityEvaluation: 1;				// ����������1������  0����
unsigned long	ComuQuality 	: 1;				//ͨ��������1������  0����
unsigned long	RxMessageFlag	: 1;				//1������  0��δ����
} PowerModuleErrorBit;


//��緽ʽ����
enum 
{
ReservedCharge = 0, 
TurnlistCharge, 
SimultanouslyCharge
};


typedef union 
{
uint32_t		all;
LongWordBytes	Byte;
PowerModuleErrorBit bit;


struct 
{
union 
{
unsigned char	all;								//???????��


struct 
{
unsigned char	OutPutShortProtect: 1;				//1: �����·
unsigned char	Rsvd0			: 3;
unsigned char	ModuleHibernate : 1;				//1:ģ������
unsigned char	ModuleOutPutAbnormal: 1;			//1:ģ��ŵ��쳣
unsigned char	Rsvd1			: 2;
} bit;


} StateTable0;


union 
{
unsigned char	all;								//???????��


struct 
{
unsigned char	ModulePowerOff	: 1;				//1��ģ�� DC �ദ�ڹػ�״̬ δ����
unsigned char	ModuleFault 	: 1;				//1��ģ����ϸ澯
unsigned char	ModuleProtect	: 1;				//1:ģ�鱣���澯 
unsigned char	FanFault		: 1;				//1�����ȹ��ϸ澯
unsigned char	OverTemp		: 1;				//1�����¸澯
unsigned char	OutOverVolt 	: 1;				//1�������ѹ�澯
unsigned char	WALKINEnable	: 1;				//1��WALK-IN ʹ��			
unsigned char	CommuCutage 	: 1;				//1��ģ��ͨ���жϸ澯			δ����
} bit;


} StateTable1;


union 
{
unsigned char	all;								//???????��


struct 
{
unsigned char	ModuleLimitedPower: 1;				//1��ģ�鴦���޹���״̬
unsigned char	ModuleAddressRepetition: 1; 		//1��ģ�� ID �ظ�
unsigned char	ModuleSeverelyUnevenFlow: 1;		//1:ģ�����ز����� 
unsigned char	ThreePhaseInputMissPhaseAlarm: 1;	//1����������ȱ��澯
unsigned char	ThreePhaseInputImbalanceAlarm: 1;	//1���������벻ƽ��澯
unsigned char	InputLowVolt	: 1;				//1������Ƿѹ�澯
unsigned char	InputOverVolt	: 1;				//1�������ѹ�澯
unsigned char	ModulePFCPowerOff: 1;				//1��ģ�� PFC �ദ�ڹػ�״̬
} bit;


} StateTable2;


union 
{
unsigned char	all;								//???????��
} StateTable3;


} INFYBit;


} PowerModuleError;


//���ò�����������
typedef struct 
{
PowerModuleError Err;								//״̬���
WordByte		ModuleCurOut;						//ģ���������
WordByte		ModuleVoltOut;						//ģ�������ѹ
uint8_t 		Temp;								// ģ���¶�
} PowerModuleMonitorAck;


//���ò�����������BEG
typedef struct 
{
PowerModuleError Err;								//״̬���
LongWordByte	ModuleCurOut;						//ģ���������
LongWordByte	ModuleVoltOut;						//ģ�������ѹ

//
PowerModuleError Err_AC;							//״̬���
LongWordByte	Module_AC_AVolt;
LongWordByte	Module_AC_BVolt;
LongWordByte	Module_AC_CVolt;
LongWordByte	Module_AC_ABVolt;
LongWordByte	Module_AC_BCVolt;
LongWordByte	Module_AC_CAVolt;
LongWordByte	Module_AC_ACur;
LongWordByte	Module_AC_BCur;
LongWordByte	Module_AC_CCur;
LongWordByte	Module_AC_HZ;
LongWordByte	Module_AC_AllP;
LongWordByte	Module_AC_APower;
LongWordByte	Module_AC_BPower;
LongWordByte	Module_AC_CPower;
LongWordByte	Module_AC_AllLP;
LongWordByte	Module_AC_LAPower;
LongWordByte	Module_AC_LBPower;
LongWordByte	Module_AC_LCPower;
WordByte		DC_Sta1;
WordByte		DC_Sta2;
WordByte		AC_Sta1;

//
uint16_t		Temp;

// ģ���¶�
} PowerModuleMonitorAck_BEG;


//���ò�����������BEG
typedef struct 
{
PowerModuleError Err;								//״̬���
LongWordByte	ModuleCurOut;						//ģ���������
LongWordByte	ModuleVoltOut;						//ģ�������ѹ

//
PowerModuleError Err_AC;							//״̬���
LongWordByte	Module_AC_AVolt;
LongWordByte	Module_AC_BVolt;
LongWordByte	Module_AC_CVolt;
LongWordByte	Module_AC_ACur;
LongWordByte	Module_AC_BCur;
LongWordByte	Module_AC_CCur;
LongWordByte	Module_AC_HZ;
LongWordByte	Module_AC_AllP;
LongWordByte	Module_AC_APower;
LongWordByte	Module_AC_BPower;
LongWordByte	Module_AC_CPower;
LongWordByte	Module_AC_AllLP;
LongWordByte	Module_AC_LAPower;
LongWordByte	Module_AC_LBPower;
LongWordByte	Module_AC_LCPower;
WordByte		DC_Sta1;
WordByte		DC_Sta2;
WordByte		AC_Sta1;

//
uint16_t		Temp;

// ģ���¶�
} PowerModuleMonitorAck_WL;


//���ò�����������BEG
typedef struct 
{
LongWordByte	ModuleCurOut;						//ģ���������//4
LongWordByte	ModuleVoltOut;						//ģ�������ѹ//4

//
WordByte		Module_Num;
WordByte		Module_Run;
WordByte		Module_sleep;
WordByte		Module_Walkin;
WordByte		Module_Add;
LongWordByte	Module_DC_minVolt;					//4
LongWordByte	Module_AC_HZ;						//4
LongWordByte	Module_AC_Volt; 					//4
LongWordByte	Module_AC_PF;						//4
LongWordByte	Module_AC_LP;						//4
WordByte		Module_Word;
WordByte		Module_3P;
WordByte		Module_Only;
WordByte		Module_LPSer;
LongWordByte	Module_AC_OV1;						//4
LongWordByte	Module_AC_OV1_T;					//4
LongWordByte	Module_AC_OV2;						//4
LongWordByte	Module_AC_OV2_T;					//4;
LongWordByte	Module_AC_UV1;						//4
LongWordByte	Module_AC_UV1_T;					//4
LongWordByte	Module_AC_UV2;						//4
LongWordByte	Module_AC_UV2_T;					//4
LongWordByte	Module_HZ_OV1;						//4
LongWordByte	Module_HZ_OV1_T;					//4
LongWordByte	Module_HZ_OV2;						//4
LongWordByte	Module_HZ_OV2_T;					//4
LongWordByte	Module_HZ_UV1;						//4
LongWordByte	Module_HZ_UV1_T;					//4
LongWordByte	Module_HZ_UV2;						//4
LongWordByte	Module_HZ_UV2_T;					//4
LongWordByte	Module_AC_OV;						//4
LongWordByte	Module_AC_OV_T; 					//4
} PowerModuleMonitorSet_BEG;


////////////////////////////////////////////////AMT���/////////////////////////////////
typedef struct 
{
WordByte		PMAPhaseVolt;
WordByte		PMBPhaseVolt;
WordByte		PMCPhaseVolt;
WordByte		PMAPhaseCur;
WordByte		PMBPhaseCur;
WordByte		PMCPhaseCur;
LongWordByte	PMEnergy;
LongWordByte	PMEnergy_out;
WordByte		PMActivePower;
WordByte		PMIddlePower;
WordByte		PMPF;
WordByte		PMFq;
WordByte		PM_harmonics_V;
WordByte		PM_harmonics_I;
WordByte		grid_FAULT;
WordByte		load_FAULT;
} AMT_Info;


typedef struct 
{
LongWordBit 	WSys_sta;
WordByte		WVolt;
WordByte		WCur;
LongWordBit 	LSys_sta;
WordByte		LVolt;
WordByte		LCur;

WordByte		WActivePower;
WordByte		WIddlePower;
WordByte		WPF;
WordByte		WFq;
WordByte		LActivePower;
WordByte		LIddlePower;
WordByte		LPF;
WordByte		LFq;

WordByte		W_FAULT;
WordByte		L_FAULT;
} WL_Info;


typedef struct 
{
LongWordBit 	DEGSys_sta;
WordByte		DEGVolt;
WordByte		DEGCur;

WordByte		DEGActivePower;
WordByte		DEGIddlePower;
WordByte		DEGPF;
WordByte		DEGFq;

WordByte		DEG_FAULT;
WordByte		DEG_SYS;
WordByte		DEG_CONFIG;
WordByte		DEG_REAL;
} DEG_Info;


////////////////////////////////////////////////AMT���/////////////////////////////////end
EXTERN uint32_t PM2_Power;

//PowerModuleMonitorAck PowerModuleAck[2];
#define PCS_DCModeNum			4//���һ�������⣬��������7+1	//max:10+1


//
#define INFYDCModule_ON //SetPowerModule(INFYIDAssembleTool(Normal,SingleModuleProtocol,CofgWholeGroupModuleOnOff_INFYComd,BroadcastAddress,MonitoringAdress), INFYModule_Cofg,INFYDCModuleOn,0,0);			//瀵�拷CMode

#define INFYDCModule_OFF //SetPowerModule(INFYIDAssembleTool(Normal,SingleModuleProtocol,CofgWholeGroupModuleOnOff_INFYComd,BroadcastAddress,MonitoringAdress), INFYModule_Cofg,INFYDCModuleOff,0,0);			//锟斤拷CMode

//
EXTERN uint8	DCModeNum;

EXTERN uint8	CC2_Run;
EXTERN uint8	CC2_DC_Run; //B2408 UI����chargeѡ��ON��ť�����ã�
EXTERN uint8	CCS_AC_DC; //B2408 1:ֱ����2������

//1C3FF456
EXTERN uint8	CF_Secc_S2_OnReq;
EXTERN uint8	CF_Secc_S2Sts;
EXTERN uint8	CF_Lock_Status;
EXTERN uint8	CF_CP_Status;
EXTERN uint8	CF_Lock_Alarm;
EXTERN uint8	CR_Secc_IsolationStatus;
EXTERN uint8	CR_Evcc_PDStatus;
EXTERN uint8	CR_Secc_AC_MaxCurrent;
EXTERN uint8	CF_Secc_DCAC_ChgMode;
EXTERN uint8	EVCC_Fault_Status;
EXTERN uint16	CR_Secc_EvseOutVolt;
EXTERN uint16	CR_Secc_EVSEMaxPowerLimit;
//
EXTERN uint8			BMS_Sys_allfaul;
EXTERN uint8			OBD_ACDC_Sta1;
EXTERN uint8			OBD_ACDC_Sta2;
EXTERN uint8			OBD_ACDC_Sta3;
//
EXTERN uint8			BMS_fual_max;
//
EXTERN uint32			Chagre_once_KWH;
EXTERN uint32			Chagre_all_KWH;
EXTERN uint32			DisChagre_once_KWH;
EXTERN uint32			DisChagre_all_KWH;

////////////////////////////////////////////////////////////////

//
EXTERN PowerBmsInfo gBMSInfo;
EXTERN Bms_CellInfo gCell_Info;

EXTERN PowerModuleMonitorSet_BEG MonitorSet_BEG;
EXTERN PowerModuleMonitorAck_BEG MonitorAck_BEG[11]; //max:10+1

//
EXTERN PowerModuleMonitorAck_WL gMonitorPG[2];
EXTERN WL_Info	gWL_Info;
EXTERN DEG_Info gDEG_Info;

//
EXTERN SetIncreaseModuleCurOut gSetIncreaseModuleCurOut;
EXTERN SetIncreaseModuleVoltOut gSetIncreaseModuleVoltOut;

EXTERN AMT_Info	gAMT_Info[12];//gAMT_Info[10]----��� gAMT_Info[11]----����

//*** <<< end of Configuration section	>>> ***
#endif

