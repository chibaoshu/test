

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
// 閿熸枻鎷烽敓鏂ゆ嫹锟斤拷鎷风紓锟芥嫹锟?///////////////////////////////////////////////////////////////////////////////////////////////////
//8锟斤拷鎷烽敓?
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


//锟斤拷鎷烽敓鐣岋拷閿熸枻锟?
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


//閿熻棄锟介敓鑺ユ媴閿熸枻鎷峰☉锟芥嫹
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


//閿熻棄锟介敓鏂ゆ嫹閿熸枻鎷峰☉锟芥嫹
typedef struct 
{
unsigned char	Byte0			: 8;
unsigned char	Byte1			: 8;
} WordBytes;


//4 锟斤拷鎷烽敓鐣岋拷閿熸枻锟?
typedef union 
{
unsigned short	all;
WordBytes		Byte;
unsigned char	Bytes[2];
} WordByte;


//閿熻棄锟介敓鏂ゆ嫹閿熸枻鎷峰☉锟芥嫹
typedef struct 
{
unsigned char	Byte0			: 8;
unsigned char	Byte1			: 8;
unsigned char	Byte2			: 8;
unsigned char	Byte3			: 8;
} LongWordBytes;


//4 锟斤拷鎷烽敓鐣岋拷閿熸枻锟?
typedef union 
{
unsigned long	all;
LongWordBytes	Byte;
uint8_t 		Array[4];
} LongWordByte;


//閿熸枻鎷风花鎸庯拷閿熷�燂拷閿熸枻锟?
typedef struct 
{
unsigned char	Byte0			: 8;
unsigned char	Byte1			: 8;
unsigned char	Byte2			: 8;
unsigned char	Byte3			: 8;
} SingleFloatBytes;


//4 锟斤拷鎷烽敓鐣岋拷閿熸枻锟?
typedef union 
{
float			all;
SingleFloatBytes Byte;
} SingleFloatByte;


/**********************************************************************
数据类型定义
***********************************************************************/
//麦格米特ID类型
typedef struct 
{
unsigned char	ModuleAddr		: 7;				//源地址
unsigned long	FrameId 		: 22;				//目标地址
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
} SetIncreaseModuleVoltOut; //设置模块电压


//设置电流类型
typedef union 
{
uint32_t		all;
LongWordByte	CurOut;
} SetIncreaseModuleCurOut; //设置模块电流


////////////////////////////BMS////////////////////
typedef union 
{
uint16_t		all;
WordByte		VoltOut;
} BmsVolt; //设置模块电压


//设置电流类型
typedef union 
{
uint16_t		all;
WordByte		CurOut;
} BmsCur; //设置模块电流


//
typedef struct 
{
WordByte		BMSCur; 							//模块输出电流
WordByte		BMSVolt;							//模块输出电压

//
WordByte		Bms_temp;							//状态输出
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
//英飞源ID类型
#define MonitoringAdress		0xF0	  //上级监控的地址
#define BroadcastAddress		0x3F	  //广播地址


typedef union 
{
unsigned long	all;


struct 
{
unsigned long	SourceAddr		: 8;				//源地址
unsigned long	TargetAddr		: 8;				//目标地址
unsigned long	ComdCode		: 6;				//命令号
unsigned long	EquipmentCode	: 4;				//设备号
unsigned long	ErrorCode		: 3;				//错误码
unsigned long	Rsvd			: 3;
} bit;


} INFYModuleID;


//错误码：表示数据信息错误原因：
enum 
{
Normal = 0, 										//正常
ComdCodeAbnormal = 2,								//命令号异常
DataInformationAbnormal = 3,						//数据信息异常
AdressInvalid = 4,									//地址无效
StartupProcedure = 7								//启动过程
};


//设备号: 用来确定协议之间的设备定义:
enum 
{
SingleModuleProtocol = 0x0A,						//监控与单个整流模块之间协议
WholeGroupModuleProtocol = 0x0B 					//监控与整组整流模块之间协议
};


enum 
{
INFYModule_Read = 0,								//读英飞源模块
INFYModule_Cofg 									//配置英飞源模块
};


//命令列表
enum 
{
ReadSysOutPut_Flaot_INFYComd = 0x01,				//读系统输出 浮点数
ReadSysModuleNum_INFYComd = 0x02,					//读模块数
ReadSingleOutPut_Flaot_INFYComd = 0x03, 			//读模块N输出 浮点数
ReadModuleStatus_INFYComd = 0x04,					//读取模块状态
ReadSingleInPut_INFYComd = 0x06,					//读取模块输入
ReadSysOutPut_INFYComd = 0x08,						//读取系统电压电流
ReadSingleModuleOutPut_INFYComd = 0x09, 			//读取单模块电压电流
ReadSingleModuleInformation_INFYComd = 0x0A,		//读取模块N信息
CofgModuleWalk_INFYComd = 0x13, 					//设置模块NWALK使能禁止
CofgModuleLed_INFYComd = 0x14,						//设置模块灯控
CofgModuleSysNum_INFYComd = 0x16,					//设置模块N组号
CofgSingleModuleHibernate_INFYComd = 0x19,			//设置单模块休眠
CofgWholeGroupModuleOnOff_INFYComd = 0x1A,			//控制所有模块开关机
CofgWholeGroupModuleOutPut_INFYComd = 0x1B, 		//设置所有模块输出
CofgSingleModuleOutPut_INFYComd = 0x1C, 			//设置单模块输出
CofgModuleReset_INFYComd = 0x1E,					//设置单模块复位
CofgModuleAdress_INFYComdTypeDistribute = 0x1F, 	//设置模块地址分配方式
};


//宏定义
//模块开关关键字
enum 
{
INFYDCModuleOn = 0, 
INFYDCModuleOff = 1
};


//英飞源电源模块控制过程
enum 
{
PeriodConfigINFYModule = 0, 						//周期配置模块参数
PeriodReadINFYModuleBack							//周期回读模块参数
};


/**********************************************************************
* Power module share internal RAM.
***********************************************************************/
//锟斤拷锟介敓鐣岋拷閿熸枻锟?
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


//濡ょ櫢鎷烽敓鐣岀尵瀹曪拷鎷稩D缂侊拷锟介敓?
typedef struct 
{
unsigned char	ModuleAddr		: 7;				//婵犙嶆嫹閿熸枻鎷烽敓?
unsigned long	FrameId 		: 22;				//閿熸枻鎷烽敓鏂ゆ嫹锟藉尅锟?
unsigned char	Rsvd			: 3;
} MegmeetModuleBit;


//濡ょ櫢鎷烽敓鐣岀尵瀹曪拷鎷烽敓鍊燂拷閿熺晫锟介敓锟界紒锟斤拷锟?
typedef struct 
{
unsigned char	RES2			: 1;				//濡府鎷烽敓鑺ユ媴閿熸枻鎷烽敓鏂ゆ嫹锟借埖锟介敓?
unsigned char	RES1			: 1;				//濡府鎷烽敓鑺ユ媴閿熸枻鎷烽敓鏂ゆ嫹锟借埖锟介敓?
unsigned char	CNT 			: 1;				//0锟斤拷鎷凤拷锟斤拷锟斤拷鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓鏂ゆ嫹1锟斤拷鎷凤拷锟斤拷锟斤拷锟介敓鏂ゆ嫹閿熸枻鎷烽敓?
unsigned char	SRCADDR 		: 8;				//婵犙嶆嫹閿熸枻鎷烽敓?
unsigned char	DSTADDR 		: 8;				//閿熸枻鎷烽敓鏂ゆ嫹锟藉尅锟?
unsigned char	PTP 			: 1;				//閿熺晫锟介敓鏂ゆ嫹楠炲尅锟?锟斤拷鎷烽敓鏂ゆ嫹閿熸枻鎷凤拷闈涳拷閿熶粙锟介敓浠嬶拷閿熸枻鎷凤拷闈涳拷锟?
unsigned short	PROTNO			: 9;				//閿熸枻鎷烽敓鏂ゆ嫹閿燂拷otocol No锟斤拷鎷穢000-0x1ff锟斤拷鎷烽敓鏂ゆ嫹閿熸枻鎷锋锟芥嫹閿熷锟斤拷锟斤拷閿熸枻鎷风紒鐙欙拷鎷凤拷锟芥嫹锟斤拷鎷烽敓鏂ゆ嫹閿熸枻鎷烽敓锟?
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


//濡ょ櫢鎷烽敓鐣岀尵瀹曪拷鎷烽敓鍊燂拷閿熺晫锟介敓锟界紒锟斤拷锟?//閿熺晫锟介敓鐣岀尵鐠囪鎷烽敓鏂ゆ嫹锟?//閿熸枻鎷烽敓锟紻閿熸枻鎷烽敓?
typedef union 
{
IncreaseModuleID IncreaseID;
MegmeetModuleID MegmeetID;
MegmeetModuleExtraID MegmeetExtraID;				//閿熷�燂拷閿熺晫锟介敓?
INFYModuleID	INFYID;
} PowerModuleID;


//锟斤拷锟斤拷锟芥嫹锟斤拷鎷风紒锟斤拷锟?
typedef union 
{
uint32_t		all;
LongWordByte	CurOut;
} SetMegmeetModuleCurOut; //锟斤拷锟斤拷锟轿燂拷绛规嫹閿熻棄锟介敓?


//锟斤拷锟斤拷锟芥嫹锟斤拷鎷风紒锟斤拷锟?
typedef union 
{
uint32_t		all;
LongWordByte	VoltOut;
} SetMegmeetModuleVoltOut; //锟斤拷锟斤拷锟轿燂拷绛规嫹閿熻棄锟介敓?


//锟斤拷锟斤拷锟芥嫹锟斤拷鎷风紒锟斤拷锟?
typedef union 
{
SetIncreaseModuleCurOut IncreaseCurOut;
SetMegmeetModuleCurOut MegmeetCurOut;
} SetPowerModuleCurOut; //锟斤拷锟斤拷锟轿燂拷绛规嫹閿熻棄锟介敓?


//锟斤拷锟斤拷锟芥嫹锟斤拷鎷风紒锟斤拷锟?
typedef union 
{
SetIncreaseModuleVoltOut IncreaseVoltOut;
SetMegmeetModuleVoltOut MegmeetVoltOut;
} SetPowerModuleVoltOut;


///故障结构构造
typedef struct 
{
unsigned long	ModuleAct		: 1;				//1：模块关机：0：模块开机
unsigned long	ModuleState 	: 1;				//1：模块故障；0：模块正常
unsigned long	OutMode 		: 1;				//1: 限流；限压
unsigned long	FanState		: 1;				//1：风扇故障；0：风扇正常
unsigned long	InOverVolt		: 1;				//1: 输入过压；0：输入正常
unsigned long	InUnderVolt 	: 1;				//1: 输入欠压；0：输入正常
unsigned long	OutOverVolt 	: 1;				//1: 输出过压；0：0：输入正常
unsigned long	OutUnderVolt	: 1;				//1: 输出欠压；输入正常
unsigned long	OverCurrentProtect: 1;				//1：过流保护；0：正常
unsigned long	OverTemperatureProtect: 1;			//1：过温保护；0：正常
unsigned long	SetSwitchModule : 1;				//1: 设置开机；0：设置关机
unsigned long	PFCOverVolt 	: 1;				//1: PFC过压；0：正常
unsigned long	PFCUnderVolt	: 1;				//1: PFC欠压；0：正常
unsigned long	ShortProtect	: 1;				//1: 短路保护；0：正常
unsigned long	Other			: 15;
unsigned long	QualityEvaluation: 1;				// 质量评估：1：正常  0：损坏
unsigned long	ComuQuality 	: 1;				//通信质量：1：正常  0：损坏
unsigned long	RxMessageFlag	: 1;				//1：接受  0；未接受
} PowerModuleErrorBit;


//充电方式定义
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
unsigned char	all;								//???????λ


struct 
{
unsigned char	OutPutShortProtect: 1;				//1: 输出短路
unsigned char	Rsvd0			: 3;
unsigned char	ModuleHibernate : 1;				//1:模块休眠
unsigned char	ModuleOutPutAbnormal: 1;			//1:模块放电异常
unsigned char	Rsvd1			: 2;
} bit;


} StateTable0;


union 
{
unsigned char	all;								//???????λ


struct 
{
unsigned char	ModulePowerOff	: 1;				//1：模块 DC 侧处于关机状态 未处理
unsigned char	ModuleFault 	: 1;				//1：模块故障告警
unsigned char	ModuleProtect	: 1;				//1:模块保护告警 
unsigned char	FanFault		: 1;				//1：风扇故障告警
unsigned char	OverTemp		: 1;				//1：过温告警
unsigned char	OutOverVolt 	: 1;				//1：输出过压告警
unsigned char	WALKINEnable	: 1;				//1：WALK-IN 使能			
unsigned char	CommuCutage 	: 1;				//1：模块通信中断告警			未处理
} bit;


} StateTable1;


union 
{
unsigned char	all;								//???????λ


struct 
{
unsigned char	ModuleLimitedPower: 1;				//1：模块处于限功率状态
unsigned char	ModuleAddressRepetition: 1; 		//1：模块 ID 重复
unsigned char	ModuleSeverelyUnevenFlow: 1;		//1:模块严重不均流 
unsigned char	ThreePhaseInputMissPhaseAlarm: 1;	//1：三相输入缺相告警
unsigned char	ThreePhaseInputImbalanceAlarm: 1;	//1：三相输入不平衡告警
unsigned char	InputLowVolt	: 1;				//1：输入欠压告警
unsigned char	InputOverVolt	: 1;				//1：输入过压告警
unsigned char	ModulePFCPowerOff: 1;				//1：模块 PFC 侧处于关机状态
} bit;


} StateTable2;


union 
{
unsigned char	all;								//???????λ
} StateTable3;


} INFYBit;


} PowerModuleError;


//设置参数数据类型
typedef struct 
{
PowerModuleError Err;								//状态输出
WordByte		ModuleCurOut;						//模块输出电流
WordByte		ModuleVoltOut;						//模块输出电压
uint8_t 		Temp;								// 模块温度
} PowerModuleMonitorAck;


//设置参数数据类型BEG
typedef struct 
{
PowerModuleError Err;								//状态输出
LongWordByte	ModuleCurOut;						//模块输出电流
LongWordByte	ModuleVoltOut;						//模块输出电压

//
PowerModuleError Err_AC;							//状态输出
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

// 模块温度
} PowerModuleMonitorAck_BEG;


//设置参数数据类型BEG
typedef struct 
{
PowerModuleError Err;								//状态输出
LongWordByte	ModuleCurOut;						//模块输出电流
LongWordByte	ModuleVoltOut;						//模块输出电压

//
PowerModuleError Err_AC;							//状态输出
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

// 模块温度
} PowerModuleMonitorAck_WL;


//设置参数数据类型BEG
typedef struct 
{
LongWordByte	ModuleCurOut;						//模块输出电流//4
LongWordByte	ModuleVoltOut;						//模块输出电压//4

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


////////////////////////////////////////////////AMT电表/////////////////////////////////
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


////////////////////////////////////////////////AMT电表/////////////////////////////////end
EXTERN uint32_t PM2_Power;

//PowerModuleMonitorAck PowerModuleAck[2];
#define PCS_DCModeNum			4//最后一组是虚拟，汇总数据7+1	//max:10+1


//
#define INFYDCModule_ON //SetPowerModule(INFYIDAssembleTool(Normal,SingleModuleProtocol,CofgWholeGroupModuleOnOff_INFYComd,BroadcastAddress,MonitoringAdress), INFYModule_Cofg,INFYDCModuleOn,0,0);			//鐎碉拷鎷稢Mode

#define INFYDCModule_OFF //SetPowerModule(INFYIDAssembleTool(Normal,SingleModuleProtocol,CofgWholeGroupModuleOnOff_INFYComd,BroadcastAddress,MonitoringAdress), INFYModule_Cofg,INFYDCModuleOff,0,0);			//閿熸枻鎷稢Mode

//
EXTERN uint8	DCModeNum;

EXTERN uint8	CC2_Run;
EXTERN uint8	CC2_DC_Run; //B2408 UI操作charge选项ON按钮（借用）
EXTERN uint8	CCS_AC_DC; //B2408 1:直流；2：交流

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

EXTERN AMT_Info	gAMT_Info[12];//gAMT_Info[10]----输出 gAMT_Info[11]----输入

//*** <<< end of Configuration section	>>> ***
#endif

