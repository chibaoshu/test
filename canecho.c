
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <libgen.h>
#include <getopt.h>
#include <limits.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/uio.h>
#include <sys/ioctl.h>
#include <net/if.h>

#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/time.h>

//
#include <GB_BWT.h>
//
#include "SmartEnergy.h"
#include "canecho.h"
///////////////////////////////????////////////////////////////////////////
#include "AMT_Moni.h"
///////////////////////////////////////////////////////////////////////
//
#define msleep(n)				usleep(n*1000)

//
extern int		optind, opterr, optopt;

//
extern const unsigned int MODENDABLECAS[4];
extern const unsigned int MODENDABLECAS_T[4];
extern Power_batt_pack_V power_batt_pack_V[CELL_NUM];
int 			running = 1;

struct can_frame frame0;
struct can_frame frame1;
struct ifreq ifr[2];
struct sockaddr_can addr[2];
char *			intf_name[2];
int 			family = PF_CAN, type = SOCK_RAW, proto = CAN_RAW;
int 			nbytes, i, out;
int 			s[2];
int 			verbose = 0;

//
#define FRAME					256

struct can_frame can0_frame[FRAME + 1];
uint16			can0_in = 0, can0_out = 0, can0_flg = 0;

//
struct can_frame can1_frame[FRAME + 1];
uint16			can1_in = 0, can1_out = 0, can1_flg = 0;

////////////////////////////////////////////////////////////////
//
uint8			DCModeNum = PCS_DCModeNum;

////////////////////////////////////////////////////////////////
uint8			CC2_Run = 0;
uint8			CC2_DC_Run = 0; //B2408 UI操作charge选项ON按钮（借用）
uint8			CCS_AC_DC = 0; //B2408 1:直流；2：交流


//1C3FF456
uint8			CF_Secc_S2_OnReq;
uint8			CF_Secc_S2Sts;
uint8			CF_Lock_Status;
uint8			CF_CP_Status;
uint8			CF_Lock_Alarm;
uint8			CR_Secc_IsolationStatus;
uint8			CR_Evcc_PDStatus;
uint8			CR_Secc_AC_MaxCurrent;
uint8			CF_Secc_DCAC_ChgMode;
uint8			EVCC_Fault_Status;
uint16			CR_Secc_EvseOutVolt;
uint16			CR_Secc_EVSEMaxPowerLimit;

////////////////////////////////////////////////////////////////
uint8			BMS_Sys_allfaul = 0;

////////////////////////////////////////////////////////////////
uint8			OBD_ACDC_Sta1 = 0;
uint8			OBD_ACDC_Sta2 = 0;
uint8			OBD_ACDC_Sta3 = 0;

//
uint8			BMS_fual_max = 0;

//
uint32			Chagre_once_KWH = 0;
uint32			Chagre_all_KWH = 0;
uint32			DisChagre_once_KWH = 0;
uint32			DisChagre_all_KWH = 0;

////////////////////////////////////////////////////////////////
//
int can_send(char can, struct can_frame frame1, unsigned char id)
{
	if (id == 1)
		{
		frame1.can_id		&= CAN_EFF_MASK;
		frame1.can_id		|= CAN_EFF_FLAG;
		}
	else 
		{
		frame1.can_id		&= CAN_SFF_MASK;
		}

	write(s[can], &frame1, sizeof(frame1));
	return 0;
}


//
extern void gb_data_set(int cmd);

//
void sigterm(int signo)
{
	//running			= 0;
	running 			= 1;
}


int can_ini()
{

	signal(SIGTERM, sigterm);
	signal(SIGHUP, sigterm);
	signal(SIGINT, sigterm);

	//
	system("ifconfig can0 down");
	usleep(500000);

	//system("ifconfig can0 down");
	//usleep(500000);
	system("ip link set can0 up type can bitrate 125000 triple-sampling on"); //B2408
	usleep(500000);

	//system("ip link set can0 up type can bitrate 250000 triple-sampling on");
	//usleep(500000);
	system("ifconfig can0 up");
	usleep(500000);

	//system("ifconfig can0 up");
	//usleep(500000);
	//
	system("ifconfig can1 down");

	//usleep(500000);
	//system("ifconfig can1 down");
	usleep(500000);
	system("ip link set can1 up type can bitrate 125000 triple-sampling on");
	usleep(500000);

	//system("ip link set can1 up type can bitrate 125000 triple-sampling on");
	//usleep(500000);
	system("ifconfig can1 up");

	//usleep(500000);
	//system("ifconfig can1 up");
	//usleep(500000);
	//usleep(500000);
	//
	intf_name[0]		= "can0";
	intf_name[1]		= "can1";

	//
	printf("interface-1 = %s, interface-2 = %s, family = %d, type = %d, proto = %d\n", 
		intf_name[0], intf_name[1], family, type, proto);

	for (i = 0; i <= 1; i++)
		{
		//
		//usleep(500000);
		//
		if ((s[i] = socket(family, type, proto)) < 0)
			{
			perror("socket");
			return 1;
			}

		addr[i].can_family	= family;
		strcpy(ifr[i].ifr_name, intf_name[i]);
		ioctl(s[i], SIOCGIFINDEX, &ifr[i]);
		addr[i].can_ifindex = ifr[i].ifr_ifindex;

		//
		//usleep(500000);
		if (bind(s[i], (struct sockaddr *) &addr[i], sizeof(addr)) < 0)
			{
			perror("bind");
			return 1;
			}
		}

	return 0;
}


//
int can0_run()
{
	//while (running)
	while (1)
		{
		if ((nbytes = read(s[0], &frame0, sizeof(frame0))) < 0)
			{
			//perror("read");
			//return 1;
			//can_ini();
			}

		//if (1)
		else 
			{
			if (can0_flg < FRAME)
				{
				if (frame0.can_id & CAN_EFF_FLAG)
					{
					frame0.can_id		= frame0.can_id & CAN_EFF_MASK;
					}
				else 
					{
					frame0.can_id		= frame0.can_id & CAN_SFF_MASK;
					}

				//printf("%04x: ", frame0.can_id);
				if (frame0.can_id & CAN_RTR_FLAG)
					{
					//printf("remote request 0");
					}
				else 
					{
					//printf("0:[%d]", frame0.can_dlc);
					//for (i = 0; i < frame0.can_dlc; i++)
					//	{
					//	printf(" %02x", frame0.data[i]);
					//	}
					//printf("\n");
					}

				can0_frame[can0_in] = frame0;
				can0_in++;

				if (can0_in >= FRAME)
					can0_in = 0;

				can0_flg++;
				}
			}

		usleep(1000);
		}
}


//////////////////////////////////////////////////////////////////////////////
struct can_frame BMSsend;

//
void Bms_Info(void) //
{
	//struct can_frame BMSsend;
	BMSsend.can_dlc 	= 8;
	BMSsend.can_id		= 0x4200;
	BMSsend.data[0] 	= 0;
	BMSsend.data[1] 	= 0;
	BMSsend.data[2] 	= 0;
	BMSsend.data[3] 	= 0;
	BMSsend.data[4] 	= 0;
	BMSsend.data[5] 	= 0;
	BMSsend.data[6] 	= 0;
	BMSsend.data[7] 	= 0;
	can_send(0, BMSsend, 1);
}


//
void Bms_Cell_Info(void) //
{
	//struct can_frame BMSsend;
	BMSsend.can_dlc 	= 8;
	BMSsend.can_id		= 0x4200;
	BMSsend.data[0] 	= 1;
	BMSsend.data[1] 	= 0;
	BMSsend.data[2] 	= 0;
	BMSsend.data[3] 	= 0;
	BMSsend.data[4] 	= 0;
	BMSsend.data[5] 	= 0;
	BMSsend.data[6] 	= 0;
	BMSsend.data[7] 	= 0;
	can_send(0, BMSsend, 1);
}


//
void Bms_SYS_Info(void) //
{
	//struct can_frame BMSsend;
	BMSsend.can_dlc 	= 8;
	BMSsend.can_id		= 0x4200;
	BMSsend.data[0] 	= 2;
	BMSsend.data[1] 	= 0;
	BMSsend.data[2] 	= 0;
	BMSsend.data[3] 	= 0;
	BMSsend.data[4] 	= 0;
	BMSsend.data[5] 	= 0;
	BMSsend.data[6] 	= 0;
	BMSsend.data[7] 	= 0;
	can_send(0, BMSsend, 1);
}


void Charging_Info(void) //
{
	//struct can_frame BMSsend;
	BMSsend.can_dlc 	= 8;
	BMSsend.can_id		= 0x184200FF;
	BMSsend.data[0] 	= CDZ_charge_start;
	BMSsend.data[1] 	= CDZ_charge_stop;

	//
	if ((SmartEnerg_sta == SmartEnergy_chager_cell) || (SmartEnerg_sta == SmartEnergy_chager_pcs))
		{
		BMSsend.data[2] 	= gBMSInfo.BMSCur_dischge_max.Bytes[0];
		BMSsend.data[3] 	= gBMSInfo.BMSCur_dischge_max.Bytes[1];
		}
	else 
		{
		if (gBMSInfo.BMSVolt.all != 0)
			{
			if ((gBMSInfo.BMSCur_dischge_max.all - (gAMT_Info[8].PMActivePower.all * 10 / (gBMSInfo.BMSVolt.all))) >=
				 0)
				{
				BMSsend.data[2] 	=
					 (unsigned char) (gBMSInfo.BMSCur_dischge_max.all - (gAMT_Info[8].PMActivePower.all * 10 / (gBMSInfo.BMSVolt.all)));
				BMSsend.data[3] 	=
					 (unsigned char) ((gBMSInfo.BMSCur_dischge_max.all - (gAMT_Info[8].PMActivePower.all * 10 / (gBMSInfo.BMSVolt.all))) >> 8);
				}
			else 
				{
				BMSsend.data[2] 	= 0;
				BMSsend.data[3] 	= 0;
				}
			}
		}

	BMSsend.data[4] 	= 0;
	BMSsend.data[5] 	= 0;
	BMSsend.data[6] 	= 0;
	BMSsend.data[7] 	= 0;
	can_send(0, BMSsend, 1);
} //


void Bms_Info_Ack(void) //
{

}


void Bms_Awake(unsigned char Sleep) //0x55:Sleep;0xaa:Awake;
{
	//struct can_frame BMSsend;
	BMSsend.can_dlc 	= 8;
	BMSsend.can_id		= 0x8201;
	BMSsend.data[0] 	= Sleep;
	BMSsend.data[1] 	= 0;
	BMSsend.data[2] 	= 0;
	BMSsend.data[3] 	= 0;
	BMSsend.data[4] 	= 0;
	BMSsend.data[5] 	= 0;
	BMSsend.data[6] 	= 0;
	BMSsend.data[7] 	= 0;
	can_send(0, BMSsend, 1);
}


void Bms_Command(unsigned char cmd) //1:changer ;2:dischanger
{
	//struct can_frame BMSsend;
	BMSsend.can_dlc 	= 8;
	BMSsend.can_id		= 0x8211;

	if (cmd == 1)
		{
		BMSsend.data[0] 	= 0xAA;
		BMSsend.data[1] 	= 0xAA;
		}

	if (cmd == 2)
		{
		BMSsend.data[0] 	= 0x0;
		BMSsend.data[1] 	= 0xAA;
		}

	if (cmd == 3)
		{
		BMSsend.data[0] 	= 0xAA;
		BMSsend.data[1] 	= 0x0;
		BMSsend.data[2] 	= CCS_AC_DC;			//B2406新增
		}

	if (cmd == 4)
		{
		BMSsend.data[0] 	= 0x0;
		BMSsend.data[1] 	= 0x0;
		}



	BMSsend.data[2] 	= CCS_AC_DC;				//B2406新增
	BMSsend.data[3] 	= 0;
	BMSsend.data[4] 	= 0;
	BMSsend.data[5] 	= 0;
	BMSsend.data[6] 	= 0;
	BMSsend.data[7] 	= 0;
	can_send(0, BMSsend, 1);
}


void Temporary_masking(void)
{
	//struct can_frame BMSsend;
	BMSsend.can_dlc 	= 8;
	BMSsend.can_id		= 0x8241;
	BMSsend.data[0] 	= 0XAA;
	BMSsend.data[1] 	= 0;
	BMSsend.data[2] 	= 0;
	BMSsend.data[3] 	= 0;
	BMSsend.data[4] 	= 0;
	BMSsend.data[5] 	= 0;
	BMSsend.data[6] 	= 0;
	BMSsend.data[7] 	= 0;
	can_send(0, BMSsend, 1);
}


///////////////
//
int can1_run()
{
	//while (running)
	while (1)
		{

		//
		if ((nbytes = read(s[1], &frame1, sizeof(frame1))) < 0)
			{
			perror("read");

			//return 1;
			//can_ini();
			}

		//if (1)
		else 
			{
			if (can1_flg < FRAME)
				{
				if (frame1.can_id & CAN_EFF_FLAG)
					{
					frame1.can_id		= frame1.can_id & CAN_EFF_MASK;
					}
				else 
					{
					frame1.can_id		= frame1.can_id & CAN_SFF_MASK;
					}

				//printf("%04x: ", frame1.can_id);
				if (frame1.can_id & CAN_RTR_FLAG)
					{
					//printf("remote request 1");
					}
				else 
					{
					//printf("1:[%d]", frame1.can_dlc);
					//for (i = 0; i < frame1.can_dlc; i++)
					//	{
					//	printf(" %02x", frame1.data[i]);
					//	}
					can1_frame[can1_in] = frame1;


					can1_in++;
					can1_flg++;

					if (can1_in >= FRAME)
						can1_in = 0;

					//printf("\n");
					}
				}

			}

		usleep(1000);
		}
}


//
//
struct can_frame can0_event;

//
PowerBmsInfo	gBMSInfo;
Bms_CellInfo	gCell_Info;

//
AMT_Info		gAMT_Info[12];//gAMT_Info[10]----输出 gAMT_Info[11]----输入


//
unsigned char	BCU_COM_num = 0;

//
extern char 	g_ucSim900ManulRecvFlag;

//
extern volatile uint32_t timer_tick_count_[100];

//
int can0_data()
{
	unsigned long	CanID;
	unsigned short	index;

	//	unsigned long	Mileage;
	unsigned int	id_num, bmu_num, temp_num;
	unsigned char	alm_num1, alm_num2, alm_num3, i;

	//
#if _BWC
	unsigned long	Energy;
	static unsigned char bwc_sta[4] =
		{
		0
		};
#endif

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	//printf("%04x: ", frame0.can_id);
	//for (i = 0; i < frame0.can_dlc; i++)
	//	{
	//	printf(" %02x", frame0.data[i]);
	//	}
	CanID				= can0_event.can_id;

	/*
		if (can0_event.can_id & CAN_EFF_FLAG)
			{
			CanID				= can0_event.can_id & CAN_EFF_MASK;
			}
		else 
			{
			CanID				= can0_event.can_id & CAN_SFF_MASK;
			}
	*/
	///////////////////////////////////////////////////////////////////////////////
	if (g_ucSim900ManulRecvFlag != 0) //if(0)
		{
		gb_vehicle_DATA.gb_Vehicle_DATA.All_v[0] = gBMSInfo.BMSVolt.Bytes[1];
		gb_vehicle_DATA.gb_Vehicle_DATA.All_v[1] = gBMSInfo.BMSVolt.Bytes[0];

		gb_vehicle_DATA.gb_Vehicle_DATA.All_i[0] = (gBMSInfo.BMSCur.all - 20000) >> 8;
		gb_vehicle_DATA.gb_Vehicle_DATA.All_i[1] = (gBMSInfo.BMSCur.all - 20000);

		gb_vehicle_DATA.gb_Vehicle_DATA.car_sta = gBMSInfo.BMS_Sta;

		gb_vehicle_DATA.gb_Vehicle_DATA.soc = gBMSInfo.SOC;

		//printf("\ngb_vehicle_DATA.gb_Vehicle_DATA.soc: %d \n", gb_vehicle_DATA.gb_Vehicle_DATA.soc);
		gb_extremum_un.gb_stextremum.extremum_maxv[0] = gBMSInfo.BMS_CELL_MAX.Bytes[1];
		gb_extremum_un.gb_stextremum.extremum_maxv[1] = gBMSInfo.BMS_CELL_MAX.Bytes[0];
		gb_extremum_un.gb_stextremum.extremum_minv[0] = gBMSInfo.BMS_CELL_MIN.Bytes[1];
		gb_extremum_un.gb_stextremum.extremum_minv[1] = gBMSInfo.BMS_CELL_MIN.Bytes[0];

		gb_extremum_un.gb_stextremum.extremum_maxt_num = 0;
		gb_extremum_un.gb_stextremum.extremum_maxt_box = 1;
		gb_extremum_un.gb_stextremum.extremum_maxt = (gBMSInfo.BMS_TEMP_MAX.all / 10) - 60;

		//
		gb_extremum_un.gb_stextremum.extremum_mint_num = 1;
		gb_extremum_un.gb_stextremum.extremum_mint_box = 1;
		gb_extremum_un.gb_stextremum.extremum_mint = (gBMSInfo.BMS_TEMP_MIN.all / 10) - 60;

		gb_alarm_int_bit_un.alarm_int =
			 gBMSInfo.Error | (gBMSInfo.Protection.all << 8) | (gBMSInfo.Alarm.Bytes[0] << 24);

		//
		power_cell_v.cell_num = CELL_NUM;
		power_cell_v.power_ce_data_V[0].stP_b_pack_V.P_b_pack_num = 1; //閿Ya�Yi?1閿Y浠媔?1i?1a倖i?1i?1i?1i?1閿Y鐣Oa枈閿Ya枻a嫹ae33a嫹i?1a嚖a嫹a畷i?1a嫹i?1i?1i?1閿Ye棄i?1閿Ye奩i??
		power_cell_v.power_ce_data_V[0].stP_b_pack_V.vol[0] = gb_vehicle_DATA.gb_Vehicle_DATA.All_v[0];
		power_cell_v.power_ce_data_V[0].stP_b_pack_V.vol[1] = gb_vehicle_DATA.gb_Vehicle_DATA.All_v[1];

		power_cell_v.power_ce_data_V[0].stP_b_pack_V.current[0] = gb_vehicle_DATA.gb_Vehicle_DATA.All_i[0]; //i?1i?1i?1i?1長巃枈閿Ya枻a嫹i?1i?1i?1i??
		power_cell_v.power_ce_data_V[0].stP_b_pack_V.current[1] = gb_vehicle_DATA.gb_Vehicle_DATA.All_i[1]; //i?1i?1i?1i?1長巃枈閿Ya枻a嫹i?1i?1i?1i??
		power_cell_v.power_ce_data_V[0].stP_b_pack_V.all_cell_num[0] =
			 (unsigned char) ((MODENDABLECAS[0] &0xff00) >> 8); //i?1i?1i?1i?1鏵1i?1i?1i?1i??
		power_cell_v.power_ce_data_V[0].stP_b_pack_V.all_cell_num[1] = (unsigned char) (MODENDABLECAS[0]); //i?1i?1i?1i?1鏵1i?1i?1i?1i??
		power_cell_v.power_ce_data_V[0].stP_b_pack_V.Serial[0] = 0x00; //閿Yai?1a棶i?1i?1a禒i?1a嫹i?1i?1i?1閿Y浠媔?1i?1a倖i?1i?1i?1i?1閿Y?
		power_cell_v.power_ce_data_V[0].stP_b_pack_V.Serial[1] = 0x00; //閿Yai?1a棶i?1i?1a禒i?1a嫹i?1i?1i?1閿Y浠媔?1i?1a倖i?1i?1i?1i?1閿Y?

		//
		power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_num = MODENDABLECAS[0];
		power_batt_pack_V[0].stP_b_pack_V.P_b_pack_num = 0xff;

		//
		power_cell_t.cell_num = CELL_NUM;			//閿Ya�Yi?1閿Y浠媔?1i?1a倖i?1i?1i?1i?1閿Y鐣Oa枈閿Ya枻a嫹ae33a嫹i?1a嚖a嫹a畷i?1a嫹i?1i?1i?1i?1鏵1i?1i?1i?1a嫹N
		power_cell_t.power_ce_data_T[0].stP_b_pack_T.P_b_pack_num = 1; //閿Ya�Yi?1閿Y浠媔?1i?1a倖i?1i?1i?1i?1閿Y鐣Oa枈閿Ya枻a嫹ae33a嫹i?1a嚖a嫹a畷i?1a嫹i?1i?1i?1閿Ye棄i?1閿Ye奩i??
		power_cell_t.power_ce_data_T[0].stP_b_pack_T.all_t_num[0] =
			 (unsigned char) ((MODENDABLECAS_T[0] &0xff00) >> 8); //i?1i?1i?1i?1鏵1i?1i?1i?1i??
		power_cell_t.power_ce_data_T[0].stP_b_pack_T.all_t_num[1] = (unsigned char) (MODENDABLECAS_T[0]); //i?1i?1i?1i?1鏵1i?1i?1i?1i??
		}

	///////////////////////////////////////////////////////////////////////////////B2408
	if (CanID == 0x430)
		{
		BMS_fual_max		= can0_event.data[0];

		//
		return 1;
		}

	////gAMT_Info[10]----输出 gAMT_Info[11]----输入
		if (CanID == 0x284f001)
		{
			OBD_ACDC_Sta1		= can0_event.data[5];
			OBD_ACDC_Sta2		= can0_event.data[6];
			OBD_ACDC_Sta3		= can0_event.data[7];
	
			//
			return 1;
		}
	
		if (CanID == 0x286f001)//交流输入
		{
			gAMT_Info[11].PMAPhaseVolt.all		= can0_event.data[0]*256+can0_event.data[1];
			gAMT_Info[11].PMBPhaseVolt.all		= can0_event.data[2]*256+can0_event.data[3];
			gAMT_Info[11].PMCPhaseVolt.all		= can0_event.data[4]*256+can0_event.data[5];
	
			//
			return 1;
		}
	
		if (CanID == 0x7331)
	{
		Chagre_all_KWH		=
			 can0_event.data[0] | (can0_event.data[1] << 8) | (can0_event.data[2] << 16) | (can0_event.data[3] << 24);
		gAMT_Info[11].PMEnergy.all = Chagre_all_KWH / 1000;

		//
		DisChagre_all_KWH	=
			 (can0_event.data[4] | (can0_event.data[5] << 8) | (can0_event.data[6] << 16) | (can0_event.data[7] << 24));
		gAMT_Info[11].PMEnergy.all = DisChagre_all_KWH / 1000;
		return 1;
	}

	   if (CanID == 0x7351)
	{
		Chagre_once_KWH 	=
			 can0_event.data[0] | (can0_event.data[1] << 8) | (can0_event.data[2] << 16) | (can0_event.data[3] << 24);
		DisChagre_once_KWH	=
			 can0_event.data[4] | (can0_event.data[5] << 8) | (can0_event.data[6] << 16) | (can0_event.data[7] << 24);

		//
		return 1;
	}

	///////////////////////////////////////////////////////////////////////////////
	if (CanID == 0x1C8056F4)
		{
		CC2_Run 			= 1;
		timer_tick_count_[8] = 0;

		//
		return 1;
		}

	///////////////////////////////////////////////////////////////////////////////
	if (CanID == 0x1806e5f4) //B2408
		{
		CF_CP_Status		= can0_event.data[4];
		CF_Secc_DCAC_ChgMode = can0_event.data[5];
		EVCC_Fault_Status	= can0_event.data[6];
		CR_Evcc_PDStatus	= can0_event.data[7];

		//
		return 1;
		}

	///////////////////////////////////////////////////////////////////////////////
	if (CanID == 0x430) //B2408
		{
		BMS_Sys_allfaul 	= can0_event.data[0];

		//
		return 1;
		}

	///////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////BMS_Info///////////////////////////////////////////////////////////////////////////////
	if (CanID == 0x4211)
		{
		gBMSInfo.BMSVolt.Bytes[0] = can0_event.data[0];
		gBMSInfo.BMSVolt.Bytes[1] = can0_event.data[1];
		gBMSInfo.BMSCur.Bytes[0] = can0_event.data[2];
		gBMSInfo.BMSCur.Bytes[1] = can0_event.data[3];
		gBMSInfo.Bms_temp.Bytes[0] = can0_event.data[4];
		gBMSInfo.Bms_temp.Bytes[1] = can0_event.data[5];
		gBMSInfo.SOC		= can0_event.data[6];
		gBMSInfo.SOH		= can0_event.data[7];

		//
		BCU_COM_num 		= 0;

		//
		return 1;
		}

	if (CanID == 0x4221)
		{
		gBMSInfo.BMSVolt_chager_OV.Bytes[0] = can0_event.data[0];
		gBMSInfo.BMSVolt_chager_OV.Bytes[1] = can0_event.data[1];
		gBMSInfo.BMSVolt_dischge_UV.Bytes[0] = can0_event.data[2];
		gBMSInfo.BMSVolt_dischge_UV.Bytes[1] = can0_event.data[3];
		gBMSInfo.BMSCur_chager_max.Bytes[0] = can0_event.data[4];
		gBMSInfo.BMSCur_chager_max.Bytes[1] = can0_event.data[5];
		gBMSInfo.BMSCur_dischge_max.Bytes[0] = can0_event.data[6];
		gBMSInfo.BMSCur_dischge_max.Bytes[1] = can0_event.data[7];

		//
		BCU_COM_num 		= 0;

		//
		return 1;
		}

	/*
		if (CanID == 0x42C0)
			{
			gBMSInfo.BMSVolt_chager_OV.Bytes[0] = can0_event.data[4];
			gBMSInfo.BMSVolt_chager_OV.Bytes[1] = can0_event.data[5];
			gBMSInfo.BMSVolt_dischge_UV.Bytes[0] = can0_event.data[6];
			gBMSInfo.BMSVolt_dischge_UV.Bytes[1] = can0_event.data[7];

			//gBMSInfo.BMSCur_chager_max.Bytes[0] = can0_event.data[4];
			//gBMSInfo.BMSCur_chager_max.Bytes[1] = can0_event.data[5];
			//gBMSInfo.BMSCur_dischge_max.Bytes[0] = can0_event.data[6];
			//gBMSInfo.BMSCur_dischge_max.Bytes[1] = can0_event.data[7];
			return 1;
			}
	*/
	if (CanID == 0x4231)
		{
		gBMSInfo.BMS_CELL_MAX.Bytes[0] = can0_event.data[0];
		gBMSInfo.BMS_CELL_MAX.Bytes[1] = can0_event.data[1];
		gBMSInfo.BMS_CELL_MIN.Bytes[0] = can0_event.data[2];
		gBMSInfo.BMS_CELL_MIN.Bytes[1] = can0_event.data[3];
		gBMSInfo.BMS_CELL_MAX_add.Bytes[0] = can0_event.data[4];
		gBMSInfo.BMS_CELL_MAX_add.Bytes[1] = can0_event.data[5];
		gBMSInfo.BMS_CELL_MIN_add.Bytes[0] = can0_event.data[6];
		gBMSInfo.BMS_CELL_MIN_add.Bytes[1] = can0_event.data[7];
		return 1;
		}

	if (CanID == 0x4241)
		{
		gBMSInfo.BMS_TEMP_MAX.Bytes[0] = can0_event.data[0];
		gBMSInfo.BMS_TEMP_MAX.Bytes[1] = can0_event.data[1];
		gBMSInfo.BMS_TEMP_MIN.Bytes[0] = can0_event.data[2];
		gBMSInfo.BMS_TEMP_MIN.Bytes[1] = can0_event.data[3];
		gBMSInfo.BMS_TEMP_MAX_add.Bytes[0] = can0_event.data[4];
		gBMSInfo.BMS_TEMP_MAX_add.Bytes[1] = can0_event.data[5];
		gBMSInfo.BMS_TEMP_MIN_add.Bytes[0] = can0_event.data[6];
		gBMSInfo.BMS_TEMP_MIN_add.Bytes[1] = can0_event.data[7];
		return 1;
		}

	if (CanID == 0x4251)
		{
		gBMSInfo.BMS_Sta	= can0_event.data[0];
		gBMSInfo.cyle_mun.Bytes[0] = can0_event.data[1];
		gBMSInfo.cyle_mun.Bytes[1] = can0_event.data[2];
		gBMSInfo.Error		= can0_event.data[3];
		gBMSInfo.Alarm.Bytes[0] = can0_event.data[4];
		gBMSInfo.Alarm.Bytes[1] = can0_event.data[5];
		gBMSInfo.Protection.Bytes[0] = can0_event.data[6];
		gBMSInfo.Protection.Bytes[1] = can0_event.data[7];
		return 1;
		}

	if (CanID == 0x4281)
		{
		gBMSInfo.Nochager_mark = can0_event.data[0];
		gBMSInfo.Nodischag_mark = can0_event.data[1];
		gBMSInfo.SOH		= can0_event.data[3];
		return 1;
		}

	if (CanID == 0x4291)
		{
		gBMSInfo.System_error = can0_event.data[0];
		gBMSInfo.System_error2 = can0_event.data[1];
		gBMSInfo.System_error3 = can0_event.data[2];
		gBMSInfo.System_almrm2 = can0_event.data[4];
		gBMSInfo.System_downpower = can0_event.data[6] *256 + can0_event.data[5];

		//
		return 1;
		}

	//if ((CanID >= 0x5001) && (CanID <= 0x52D1))
	if (CanID == 0x735)
		{
		id_num				= can0_event.data[0] +can0_event.data[1] *256;

		gCell_Info.BMSCell_Volt[id_num].Bytes[0] = can0_event.data[2];
		gCell_Info.BMSCell_Volt[id_num].Bytes[1] = can0_event.data[3];

		//
		gCell_Info.BMSCell_Volt[id_num + 1].Bytes[0] = can0_event.data[4];
		gCell_Info.BMSCell_Volt[id_num + 1].Bytes[1] = can0_event.data[5];

		//
		gCell_Info.BMSCell_Volt[id_num + 2].Bytes[0] = can0_event.data[6];
		gCell_Info.BMSCell_Volt[id_num + 2].Bytes[1] = can0_event.data[7];

		//
		//
		if (g_ucSim900ManulRecvFlag != 0) //if(0)
			{
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[(id_num * 2)] =
				 gCell_Info.BMSCell_Volt[id_num].Bytes[1];
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[(id_num * 2) + 1] =
				 gCell_Info.BMSCell_Volt[id_num].Bytes[0];
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[(id_num * 2) + 2] =
				 gCell_Info.BMSCell_Volt[id_num + 1].Bytes[1];
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[(id_num * 2) + 3] =
				 gCell_Info.BMSCell_Volt[id_num + 1].Bytes[0];
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[(id_num * 2) + 4] =
				 gCell_Info.BMSCell_Volt[id_num + 2].Bytes[1];
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[(id_num * 2) + 5] =
				 gCell_Info.BMSCell_Volt[id_num + 2].Bytes[0];
			}

		//
		return 1;
		}

	//if ((CanID >= 0x6001) && (CanID <= 0x67F1))
	if (CanID == 0x780)
		{
		id_num				= can0_event.data[0] +can0_event.data[1] *256;

		gCell_Info.BMSCell_Temp[id_num].Bytes[0] = can0_event.data[2];
		gCell_Info.BMSCell_Temp[id_num].Bytes[1] = can0_event.data[3];

		//
		gCell_Info.BMSCell_Temp[id_num + 1].Bytes[0] = can0_event.data[4];
		gCell_Info.BMSCell_Temp[id_num + 1].Bytes[1] = can0_event.data[5];

		gCell_Info.BMSCell_Temp[id_num + 2].Bytes[0] = can0_event.data[6];
		gCell_Info.BMSCell_Temp[id_num + 2].Bytes[1] = can0_event.data[7];

		//
		//
		if (g_ucSim900ManulRecvFlag != 0) //if(0)
			{
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.cell_voll[id_num] =
				 (gCell_Info.BMSCell_Temp[id_num].all / 10) + 40;
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.cell_voll[id_num + 1] =
				 (gCell_Info.BMSCell_Temp[id_num + 1].all / 10) + 40;
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.cell_voll[id_num + 2] =
				 (gCell_Info.BMSCell_Temp[id_num + 2].all / 10) + 40;
			}

		//
		//
		return 1;
		}

	if (CanID == 0x7341)
		{

		if (((can0_event.data[1] *256) +can0_event.data[0]) >= ((can0_event.data[3] *256) +can0_event.data[2]))
			{
			gBMSInfo.RON.all	= ((can0_event.data[3] *256) +can0_event.data[2]);
			}
		else 
			{
			gBMSInfo.RON.all	= ((can0_event.data[1] *256) +can0_event.data[0]);
			}


		//
		return 1;
		}

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////i?1鈺媋嫹閿Ya枻a嫹閿Ya枻a嫹i?1i?1i?1i??	//
	if (CanID == 0x0cff1819)
		{
		if ((can0_event.data[0] == 0x1a) && (can0_event.data[1] == 0x1b) && (can0_event.data[2] == 0x1c) &&
			 (can0_event.data[3] == 0x1d) && (can0_event.data[4] == 0x1e))
			gb_data_set(2);

		if ((can0_event.data[0] == 0x2a) && (can0_event.data[1] == 0x2b) && (can0_event.data[2] == 0x2c) &&
			 (can0_event.data[3] == 0x2d) && (can0_event.data[4] == 0x2e))
			gb_data_set(1);
		}

#if _BWC

	//
	if (CanID == 0x90) //
		{
		switch (can0_event.data[0])
			{
			case 0:
				usr_data_bwc_un.stusr_data_bwc.bwc_door1 = 0x02;
				break;

			case 0xfe:
				usr_data_bwc_un.stusr_data_bwc.bwc_door1 = 0xfe;
				break;

			default:
				usr_data_bwc_un.stusr_data_bwc.bwc_door1 = 0x01;
				break;
			}

		//
		switch (can0_event.data[1])
			{
			case 0:
				usr_data_bwc_un.stusr_data_bwc.bwc_lock = 0x02;
				break;

			case 0xfe:
				usr_data_bwc_un.stusr_data_bwc.bwc_lock = 0xfe;
				break;

			default:
				usr_data_bwc_un.stusr_data_bwc.bwc_lock = 0x01;
				break;
			}

		//
		switch (can0_event.data[2])
			{
			case 0:
				usr_data_bwc_un.stusr_data_bwc.bwc_lamp = 0x02;
				break;

			case 0xfe:
				usr_data_bwc_un.stusr_data_bwc.bwc_lamp = 0xfe;
				break;

			default:
				usr_data_bwc_un.stusr_data_bwc.bwc_lamp = 0x01;
				break;
			}

		//
		switch (can0_event.data[3])
			{
			case 0:
				usr_data_bwc_un.stusr_data_bwc.bwc_window = 0x02;
				break;

			case 0xfe:
				usr_data_bwc_un.stusr_data_bwc.bwc_window = 0xfe;
				break;

			default:
				usr_data_bwc_un.stusr_data_bwc.bwc_window = 0x01;
				break;
			}

		//
		if ((usr_data_bwc_un.stusr_data_bwc.bwc_door1 != bwc_sta[0]) ||
			 (usr_data_bwc_un.stusr_data_bwc.bwc_lock != bwc_sta[1]) ||
			 (usr_data_bwc_un.stusr_data_bwc.bwc_lamp != bwc_sta[2]) ||
			 (usr_data_bwc_un.stusr_data_bwc.bwc_window != bwc_sta[3]))
			{ //i?1i?1i?1i?1i?1i?1i?1i?1a嫹閿Ya枻a嫹i?1a嚖a嫹i?1i?1a嫹i?1i?1i?1閿Ya枻a嫹i?1i?1a嫹i?1鏞碼嫹閿Y浠媔?1i?1a倖i?1閿Yai?1閿Y浠媔?1i?1a倖i??
			bwc_sta[0]			= usr_data_bwc_un.stusr_data_bwc.bwc_door1;
			bwc_sta[1]			= usr_data_bwc_un.stusr_data_bwc.bwc_lock;
			bwc_sta[2]			= usr_data_bwc_un.stusr_data_bwc.bwc_lamp;
			bwc_sta[3]			= usr_data_bwc_un.stusr_data_bwc.bwc_window;


			gb_bwc_();

			//
			gb_bwc__flag		= 0;

			//
			//
			}
		else 
			{
			//usr_data_bwc_un.stusr_data_bwc.
			gb_bwc__flag		= 1;
			}

		//
		}

	//
	if (CanID == 0x91) //
		{
		usr_data_bwc_un.stusr_data_bwc.bwc_mileage[0] = can0_event.data[0];
		usr_data_bwc_un.stusr_data_bwc.bwc_mileage[1] = can0_event.data[1];
		usr_data_bwc_un.stusr_data_bwc.bwc_mileage[2] = can0_event.data[2];
		usr_data_bwc_un.stusr_data_bwc.bwc_mileage[3] = can0_event.data[3];

		//
		Energy				=
			 (unsigned long) (can0_event.data[4] << 24) | (unsigned long) (can0_event.data[5] << 16) | (unsigned long) (can0_event.data[6] << 8) | (unsigned long) (can0_event.data[7]);

		//
		Energy				= Energy / 100;
		usr_data_bwc_un.stusr_data_bwc.bwc_addmileage[0] = (unsigned char) (Energy >> 8);
		usr_data_bwc_un.stusr_data_bwc.bwc_addmileage[1] = (unsigned char) (Energy);
		}

	if (CanID == 0x96) //
		{

		}

	if (CanID == 0x97) //
		{

		}

	//
#endif


	///////////////////////////////////////////////////////////////////////////////
	//
	// i?1i?1i?1i?1i?1a仈閿Ya枻a嫹i?1i?1i?1i?1鏵1i?1i?1a倖i?1i?1i?1i?1閿Ye奩a偊i?1i?1i??e-Power aΔi1卛?1i?1鏵1i?1a禒i?1a嫹i?1i?1i?1閿Y浠媔?1a禒i?1a嫹
	//
	///////////////////////////////////////////////////////////////////////////////
	if (CanID == 0x18F810F0) // ?乮?1a嫹i?1i?1i?1a禒i?1a嫹i?1i?1i?1i?1長巃枈閿Ya枻a嫹i?1i?1i?1缁爄?1a嫹i?1i?1a嫹i?1i?1i?1i?1長巃枈閿Ya枻a嫹i?1i?1i?1閿Ya枻a嫹i?1i?1a嫹SOC
		{

		index				= (can0_event.data[1] *256 + can0_event.data[0]);
		gb_vehicle_DATA.gb_Vehicle_DATA.All_v[0] = (unsigned char) (index >> 8);
		gb_vehicle_DATA.gb_Vehicle_DATA.All_v[1] = (unsigned char)
		index;										// ?乮?1a嫹i?1i?1i?1a禒i?1a嫹i?1i?1i?1i?1長巃枈閿Ya枻a嫹i?1i?1i?1缁?
		index				= (((can0_event.data[3] *256 + can0_event.data[2])));
		gb_vehicle_DATA.gb_Vehicle_DATA.All_i[0] = (unsigned char) (index >> 8);
		gb_vehicle_DATA.gb_Vehicle_DATA.All_i[1] = (unsigned char)
		index;										//

		if ((index <= 10100) && (index >= 9900))
			{
			gb_vehicle_DATA.gb_Vehicle_DATA.car_sta = Flameout; //a?!i?1a?ai?1鏵1i?1a禒i?1a嫹aùi?1a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ye奩i??
			gb_vehicle_DATA.gb_Vehicle_DATA.charge_sta = Charge_over; //a?!i?1a?ai?1鏵1i?1i?1i?1i?1i?1i?1i?1i?1a嚖a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ye奩i??
			}
		else 
			{
			if (index > 10100)
				{
				//gb_vehicle_DATA.gb_Vehicle_DATA.car_sta = Start_up;//a?!i?1a?ai?1鏵1i?1a禒i?1a嫹aùi?1a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ye奩i??
				if (gb_vehicle_DATA.gb_Vehicle_DATA.car_sta == Other_sta)
					gb_vehicle_DATA.gb_Vehicle_DATA.car_sta = Other_sta; //a?!i?1a?ai?1鏵1i?1a禒i?1a嫹aùi?1a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ye奩i??
				else 
					gb_vehicle_DATA.gb_Vehicle_DATA.car_sta = Start_up; //a?!i?1a?ai?1鏵1i?1a禒i?1a嫹aùi?1a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ye奩i??

				gb_vehicle_DATA.gb_Vehicle_DATA.charge_sta = Travel; //a?!i?1a?ai?1鏵1i?1i?1i?1i?1i?1i?1i?1i?1a嚖a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ye奩i??
				}
			else 
				{
				gb_vehicle_DATA.gb_Vehicle_DATA.car_sta = Start_up; //a?!i?1a?ai?1鏵1i?1a禒i?1a嫹aùi?1a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ye奩i??

				//gb_vehicle_DATA.gb_Vehicle_DATA.car_sta = Flameout;//a?!i?1a?ai?1鏵1i?1a禒i?1a嫹aùi?1a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ye奩i??
				gb_vehicle_DATA.gb_Vehicle_DATA.charge_sta = NO_charge; //a?!i?1a?ai?1鏵1i?1i?1i?1i?1i?1i?1i?1i?1a嚖a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹閿Ye奩i??				}
				}

			// ?乮?1a嫹i?1i?1i?1a禒i?1i??SOC
			//g_sExtremInfo.SOC[0] = 0x00;
			//g_sExtremInfo.SOC[1] = (MessageCAN0.DATA[4]*4)/10 ;
			gb_vehicle_DATA.gb_Vehicle_DATA.soc = can0_event.data[4];

			//
			//aΔi?1i?1閿Y浠媔?1i?1i?1a嫹i?1i?1i?1i??
			index				= (can0_event.data[7] *256 + can0_event.data[6]);
			gb_vehicle_DATA.gb_Vehicle_DATA.nsulation_res[0] = (unsigned char) (index >> 8);
			gb_vehicle_DATA.gb_Vehicle_DATA.nsulation_res[1] = (unsigned char) (index);
			return 1;

			}
		}

	////////////////////////////////////////////////////////////////////////////////////////
	if (CanID == 0x18F811F0) // ?乮?1a嫹i?1i?1i?1a禒i?1a嫹i?1i?1i?1閿Y浠媔?1i?1i?1a嫹缁3a府a嫹閿Y浠媔?1a禒i?1i?1i?1i?1i?1i?1鏵1i?1i?1i?1i?1
		{

		//index =  MessageCAN0.DATA[0] + (MessageCAN0.DATA[1]<<8) ;
		gb_extremum_un.gb_stextremum.extremum_maxv[0] = can0_event.data[1];
		gb_extremum_un.gb_stextremum.extremum_maxv[1] = can0_event.data[0];
		gb_extremum_un.gb_stextremum.extremum_maxv_box = can0_event.data[3];
		gb_extremum_un.gb_stextremum.extremum_maxv_num = can0_event.data[2];

		//index = (MessageCAN0.DATA[3] *256 + MessageCAN0.DATA[4]) ;
		gb_extremum_un.gb_stextremum.extremum_minv[0] = can0_event.data[5];
		gb_extremum_un.gb_stextremum.extremum_minv[1] = can0_event.data[4];
		gb_extremum_un.gb_stextremum.extremum_minv_box = can0_event.data[7];
		gb_extremum_un.gb_stextremum.extremum_minv_num = can0_event.data[6];

		return 1;

		}

	if (CanID == 0x18F812F0) // ?乮?1a嫹i?1i?1i?1a禒i?1a嫹i?1i?1i?1閿Y浠媔?1i?1i?1a嫹i?1i?1i?1i?1鏵1i?1i?1i?1i??		
		{
		gb_extremum_un.gb_stextremum.extremum_maxt_num = can0_event.data[2];

		gb_extremum_un.gb_stextremum.extremum_maxt_box = can0_event.data[1];
		gb_extremum_un.gb_stextremum.extremum_maxt = can0_event.data[0];

		//
		gb_extremum_un.gb_stextremum.extremum_mint_num = can0_event.data[5];
		gb_extremum_un.gb_stextremum.extremum_mint_box = can0_event.data[4];
		gb_extremum_un.gb_stextremum.extremum_mint = can0_event.data[3];
		return 1;

		}

	//
	////////////////////////////////////////////////////////////////////////////////////
	if (CanID == 0x18F815F0) //baoji//i?1i?1i?1閿Ye奩a仈閿Ye奩i?1i?1i?1i?1i?1閿媋仚a?^鐐琲?1i?1鏞碼嫹i?1?i?1i?1i?1閿Y?
		{
		alm_num1			= 0;
		alm_num2			= 0;
		alm_num3			= 0;

		if ((can0_event.data[0] &0x03) == 0x03) // i?1i?1i?1i?1鏵1i?1a禒i?1i?1aΔi1卛?1閿Ya枻a嫹閿Ye奩i?1
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.allcell_over_v = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 3;
			alm_num3++;

			}
		else if ((can0_event.data[0] &0x03) != 0x00) //((MessageCAN0.DATA[2]&0x1c)==0x04)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.allcell_over_v = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 1;
			alm_num1++;
			}
		else if ((can0_event.data[0] &0x03) == 0x00)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.allcell_over_v = 0;
			gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
			}

		//
		if ((can0_event.data[0] &0x0C) == 0x0C) // i?1i?1i?1i?1鏵1i?1a禒i?1i?1aΔi1卛?1閿Y鐣Oa枈閿Ye奩i?1
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.allcell_low_v = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 3;
			alm_num3++;

			}
		else if ((can0_event.data[0] &0x0C) != 0x00) //((MessageCAN0.DATA[3]&0x30) == 0x10)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.allcell_low_v = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 1;
			alm_num1++;
			}
		else if ((can0_event.data[0] &0x0C) == 0x00)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.allcell_low_v = 0;
			gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
			}

		//
		if ((can0_event.data[2] &0x30) == 0x30) //aΔi?1i?1閿Y浠媔?1i?1i?1a嫹i?1i?1i?1閿Ye奩a侺i?1i?1a嫹缁3a府a嫹i?1?
			{


			gb_alarm_int_bit_un.gb_Alarm_bit.nsulation_res = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 3;
			alm_num3++;
			}
		else if ((can0_event.data[2] &0x30) != 0x00)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.nsulation_res = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 1;
			alm_num1++;
			}
		else if ((can0_event.data[2] &0x30) == 0x00)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.nsulation_res = 0;
			gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
			}

		//
		if ((can0_event.data[2] &0xC0) == 0xC0) //SOC aΔi1卛?1閿Y鐣Oa枈閿Ye奩i??
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.soc_low = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 3;
			alm_num3++;
			}
		else if ((can0_event.data[2] &0xC0) != 0x00) //((MessageCAN0.DATA[3]&0x03) == 0x01)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.soc_low = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 1;
			alm_num1++;
			}
		else if ((can0_event.data[2] &0xC0) == 0x00)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.soc_low = 0;
			gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
			}

		//
		//
		if ((can0_event.data[0] &0xc0) == 0xc0) //缁3a府a嫹i?1aY竔?1i?1af梐嫹缁3a府a嫹閿Y浠媔?1a禒i?1i?1aΔi1卛?1閿Y鐣Oa枈閿Ye奩i??
			{


			gb_alarm_int_bit_un.gb_Alarm_bit.cell_low_v = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 3;
			alm_num3++;
			}
		else if ((can0_event.data[0] &0xc0) != 0x00) //((MessageCAN0.DATA[3]&0xc0) == 0x40)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.cell_low_v = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 1;
			alm_num1++;
			}
		else if ((can0_event.data[0] &0xc0) == 0x00)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.cell_low_v = 0;
			gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
			}

		//
		if ((can0_event.data[0] &0x30) == 0x30) //缁3a府a嫹i?1aY竔?1i?1af梐嫹缁3a府a嫹閿Y浠媔?1a禒i?1i?1aΔi1卛?1閿Ya枻a嫹閿Ye奩i??
			{


			gb_alarm_int_bit_un.gb_Alarm_bit.cell_over_v = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 3;
			alm_num3++;
			}
		else if ((can0_event.data[0] &0x30) != 0x00) //((MessageCAN0.DATA[2]&0xe0)==0x20)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.cell_over_v = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 1;
			alm_num1++;
			}
		else if ((can0_event.data[0] &0x30) == 0x00)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.cell_over_v = 0;
			gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
			}

		//
		if ((can0_event.data[1] &0x30) == 0x30) //i?1i?1i?1i?1a嚖a嫹鐠嘚刬?1aΔi1卛?1閿Y鐣Oa枈閿Ye奩i??
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.temp_low = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 3;
			alm_num3++;
			}
		else if ((can0_event.data[1] &0x30) != 0x00) //((MessageCAN0.DATA[5]&0x03) == 0x01)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.temp_low = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 1;
			alm_num1++;
			}
		else if ((can0_event.data[1] &0x30) == 0x00)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.temp_low = 0;
			gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
			}

		//
		if ((can0_event.data[1] &0x0c) == 0x0C) //i?1i?1i?1i?1a嚖a嫹鐠嘚刬?1aΔi1卛?1閿Ya枻a嫹閿Ye奩i??
			{


			gb_alarm_int_bit_un.gb_Alarm_bit.cell_higt_temper = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 3;
			alm_num3++;
			}
		else if ((can0_event.data[1] &0x0c) != 0x00) //((MessageCAN0.DATA[0]&0x1c)==0x04)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.cell_higt_temper = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 1;
			alm_num1++;
			}
		else if ((can0_event.data[1] &0x0c) == 0x00)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.cell_higt_temper = 0;
			gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
			}

		//
		/////////////////////////////////////////////////////////////////////////////
		if ((can0_event.data[1] &0x03) == 0x03) //缁3a府a嫹i?1aY竔?1i?1af梐嫹i?1i?1i?1缁爄?1a嫹閿Ya枻a嫹aΔi1卛?1閿Ye棄i?1i?1i?1i??
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.cell_dif_v = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 3;
			alm_num3++;
			}
		else if ((can0_event.data[1] &0x03) != 0x00) //((MessageCAN0.DATA[4]&0x30) == 0x10)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.cell_dif_v = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 1;
			alm_num1++;
			}
		else if ((can0_event.data[1] &0x03) == 0x00)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.cell_dif_v = 0;
			gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
			}


		//
		if ((can0_event.data[1] &0xC0) == 0xC0) //i?1i?1i?1i?1a嚖a嫹閿Ya枻a嫹aΔi1卛?1閿Ye棄i?1i?1i?1i?1
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.dif_temper = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 3;
			alm_num3++;
			}
		else if (((can0_event.data[1] &0xC0)) != 0x00) //((MessageCAN0.DATA[0]&0xe0)==0x20)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.dif_temper = 1;

			//gb_alarm_un.gb_stAlarm.Alarm_leve = 1;
			alm_num1++;
			}
		else if (((can0_event.data[1] &0xC0)) == 0x00)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.dif_temper = 0;
			gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
			}

		///////////////////////////////////////////////////////////////////////////////////////
		////////////////////////////////////////////////////////////////////////////////////////////////////////
		if ((can0_event.data[2] &0x03) == 0x03) //ae33a嫹閿Y鐣Oa枈閿Ya枻a嫹缁3a府a嫹閿Y浠媔?1i?1af梐嫹aΔi1卛?1閿Ya枻a嫹閿Ye奩i?1閿Ya枻a嫹閿Ye奩a偊i?1N?i??
			{

			alm_num3++;
			gb_alarm_int_bit_un.gb_Alarm_bit.charge_over_i = 1;

			}
		else if (((can0_event.data[2] &0x03)) != 0x00) //
			{
			alm_num1++;
			gb_alarm_int_bit_un.gb_Alarm_bit.charge_over_i = 1;

			}
		else if (((can0_event.data[2] &0x03)) == 0x00)
			{

			gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
			gb_alarm_int_bit_un.gb_Alarm_bit.charge_over_i = 0;
			}

		//
		if ((can0_event.data[2] &0x0c) == 0x0c) //i?1鏞碼嫹i?1長巃枈閿Ya枻a嫹缁3a府a嫹閿Y浠媔?1i?1af梐嫹aΔi1卛?1閿Ya枻a嫹閿Ye奩i?1閿Ya枻a嫹閿Ye奩a偊i?1N?i??
			{
			alm_num3++;
			gb_alarm_int_bit_un.gb_Alarm_bit.discharge_over_i = 1;

			}
		else if (((can0_event.data[2] &0x0c)) != 0x00) //
			{
			alm_num1++;
			gb_alarm_int_bit_un.gb_Alarm_bit.discharge_over_i = 1;

			}
		else if (((can0_event.data[2] &0x0c)) == 0x00)
			{

			gb_alarm_int_bit_un.gb_Alarm_bit.discharge_over_i = 0;
			}

		//
		if ((can0_event.data[3] &0x08) == 0x08) //i?1i?1i?1閿Ye奩a仈閿Ya枻a嫹缁3a府a嫹閿Y鐣Oi?1閿Ya枻a嫹i?1i?1i?1閿Y浠媔?1i?1a倖i?1aΔi1卛?1i?1鏵1i?1i?1a倖i?1
			{
			alm_num3++;
			gb_alarm_int_bit_un.gb_Alarm_bit.CAN_data = 1;
			}
		else 
			{
			gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
			gb_alarm_int_bit_un.gb_Alarm_bit.CAN_data = 0;
			}

		//
		if ((can0_event.data[3] &0x01) == 0x01) //i?1i?1i?1閿Yi?1CUi?1i?1i?1閿Y浠媔?1i?1a倖i?1aΔi1卛?1i?1鏵1i?1i?1a倖i?1
			{
			alm_num2++;
			gb_alarm_int_bit_un.gb_Alarm_bit.CAN_data = 1;
			}
		else 
			{
			if (gb_alarm_int_bit_un.gb_Alarm_bit.CAN_data == 0)
				{
				gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
				gb_alarm_int_bit_un.gb_Alarm_bit.CAN_data = 0;
				}

			}

		//
		if ((can0_event.data[3] &0x02) == 0x02) //i?1i?1i?1閿Ye奩a仈閿Ya枻a嫹缁3a府a嫹閿Y鐣Oi?1閿Ya枻a嫹i?1i?1i?1閿Y浠媔?1i?1a倖i?1aΔi1卛?1i?1鏵1i?1i?1a倖i?1
			{
			alm_num1++;
			gb_alarm_int_bit_un.gb_Alarm_bit.CAN_data = 1;
			}
		else 
			{
			if (gb_alarm_int_bit_un.gb_Alarm_bit.CAN_data == 0)
				{
				gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
				gb_alarm_int_bit_un.gb_Alarm_bit.CAN_data = 0;
				}

			}

		//
		if ((can0_event.data[3] &0x04) == 0x04) //i?1i?1i?1閿Ye奩a仈閿Ya枻a嫹缁3a府a嫹閿Y鐣Oi?1閿Ya枻a嫹i?1i?1i?1閿Y浠媔?1i?1a倖i?1aΔi1卛?1i?1鏵1i?1i?1a倖i?1
			{
			alm_num3++;
			gb_alarm_int_bit_un.gb_Alarm_bit.Pre_charge = 1;
			}
		else 
			{
			gb_alarm_un.gb_stAlarm.Alarm_leve = 0;
			gb_alarm_int_bit_un.gb_Alarm_bit.Pre_charge = 0;
			}

		//if(alm_num2 != 0) {gb_alarm_un.gb_stAlarm.Alarm_leve = 2;gb_alarm_flag = 1;}
		////////////////////////////////////////////////////////////////////////////////////////
		if (alm_num1 != 0)
			{
			gb_alarm_un.gb_stAlarm.Alarm_leve = 1;
			gb_alarm_flag		= 1;
			}

		if (alm_num2 != 0)
			{
			gb_alarm_un.gb_stAlarm.Alarm_leve = 2;
			gb_alarm_flag		= 1;
			}

		if (alm_num3 != 0)
			{
			gb_alarm_un.gb_stAlarm.Alarm_leve = 3;
			gb_alarm_flag		= 1;
			}

		gb_alarm_un.gb_stAlarm.gb_alarm_int_bit[0] =
			 (unsigned char) ((gb_alarm_int_bit_un.alarm_int & 0xff000000) >> 24);
		gb_alarm_un.gb_stAlarm.gb_alarm_int_bit[1] =
			 (unsigned char) ((gb_alarm_int_bit_un.alarm_int & 0x00ff0000) >> 16);
		gb_alarm_un.gb_stAlarm.gb_alarm_int_bit[2] =
			 (unsigned char) ((gb_alarm_int_bit_un.alarm_int & 0x0000ff00) >> 8);
		gb_alarm_un.gb_stAlarm.gb_alarm_int_bit[3] = (unsigned char) ((gb_alarm_int_bit_un.alarm_int & 0x000000ff));
		return 1;
		}

	////////////////////////////////////////////////////////////////////////////////////////
	//
	if ((CanID & 0xffff00ff) == 0x18F800F0)
		{
		id_num				= 0;
		bmu_num 			= 0;
		temp_num			= 0;

		//
		id_num				= (unsigned char) ((CanID & 0x0000ff00) >> 8);

		//
		if ((id_num > 0x95) || (id_num < 0x17))
			return 1;

		//
		if (id_num < 0x6b)
			{
			//
			power_cell_v.cell_num = CELL_NUM;
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.P_b_pack_num = 1; //閿Ya�Yi?1閿Y浠媔?1i?1a倖i?1i?1i?1i?1閿Y鐣Oa枈閿Ya枻a嫹ae33a嫹i?1a嚖a嫹a畷i?1a嫹i?1i?1i?1閿Ye棄i?1閿Ye奩i??
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.vol[0] = gb_vehicle_DATA.gb_Vehicle_DATA.All_v[0];

			//i?1i?1i?1i?1長巃枈閿Ya枻a嫹i?1i?1i?1i??
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.vol[1] = gb_vehicle_DATA.gb_Vehicle_DATA.All_v[1];

			//i?1i?1i?1i?1長巃枈閿Ya枻a嫹i?1i?1i?1i??
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.current[0] = gb_vehicle_DATA.gb_Vehicle_DATA.All_i[0]; //i?1i?1i?1i?1長巃枈閿Ya枻a嫹i?1i?1i?1i??
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.current[1] = gb_vehicle_DATA.gb_Vehicle_DATA.All_i[1]; //i?1i?1i?1i?1長巃枈閿Ya枻a嫹i?1i?1i?1i??
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.all_cell_num[0] =
				 (unsigned char) ((MODENDABLECAS[0] &0xff00) >> 8); //i?1i?1i?1i?1鏵1i?1i?1i?1i??
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.all_cell_num[1] = (unsigned char) (MODENDABLECAS[0]); //i?1i?1i?1i?1鏵1i?1i?1i?1i??
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.Serial[0] = 0x00; //閿Yai?1a棶i?1i?1a禒i?1a嫹i?1i?1i?1閿Y浠媔?1i?1a倖i?1i?1i?1i?1閿Y?
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.Serial[1] = 0x00; //閿Yai?1a棶i?1i?1a禒i?1a嫹i?1i?1i?1閿Y浠媔?1i?1a倖i?1i?1i?1i?1閿Y?

			//
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_num = MODENDABLECAS[0];
			power_batt_pack_V[0].stP_b_pack_V.P_b_pack_num = 0xff;

			//
			if (id_num > 0x41)
				return 1;

			bmu_num 			= (id_num - 0x17) * 8;
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[bmu_num + 0] = can0_event.data[1];
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[bmu_num + 1] = can0_event.data[0];

			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[bmu_num + 2] = can0_event.data[3];
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[bmu_num + 3] = can0_event.data[2];

			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[bmu_num + 4] = can0_event.data[5];
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[bmu_num + 5] = can0_event.data[4];

			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[bmu_num + 6] = can0_event.data[7];
			power_cell_v.power_ce_data_V[0].stP_b_pack_V.cell_voll[bmu_num + 7] = can0_event.data[6];
			}
		else 
			{
			//
			power_cell_t.cell_num = CELL_NUM;		//閿Ya�Yi?1閿Y浠媔?1i?1a倖i?1i?1i?1i?1閿Y鐣Oa枈閿Ya枻a嫹ae33a嫹i?1a嚖a嫹a畷i?1a嫹i?1i?1i?1i?1鏵1i?1i?1i?1a嫹N
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.P_b_pack_num = 1; //閿Ya�Yi?1閿Y浠媔?1i?1a倖i?1i?1i?1i?1閿Y鐣Oa枈閿Ya枻a嫹ae33a嫹i?1a嚖a嫹a畷i?1a嫹i?1i?1i?1閿Ye棄i?1閿Ye奩i??
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.all_t_num[0] =
				 (unsigned char) ((MODENDABLECAS_T[0] &0xff00) >> 8); //i?1i?1i?1i?1鏵1i?1i?1i?1i??
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.all_t_num[1] = (unsigned char) (MODENDABLECAS_T[0]); //i?1i?1i?1i?1鏵1i?1i?1i?1i??

			//
			if ((id_num > 0x95) || (id_num < 0x80))
				return 1;

			bmu_num 			= (id_num - 0x80) * 8;

			///////////////////////////////////////////////////////
			temp_num			= TEMP_CHKPOT % 8;

			if (temp_num != 0)
				{
				if (bmu_num == (TEMP_CHKPOT - temp_num))
					{
					for (i = 0; i < temp_num; i++)
						power_cell_t.power_ce_data_T[0].stP_b_pack_T.cell_voll[bmu_num + i] = can0_event.data[i];

					for (i = temp_num; i < (8 - temp_num); i++)
						power_cell_t.power_ce_data_T[0].stP_b_pack_T.cell_voll[bmu_num + i] = 0x00;

					return 1;
					}
				}

			///////////////////////////////////////////////////////
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.cell_voll[bmu_num + 0] = can0_event.data[0];
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.cell_voll[bmu_num + 1] = can0_event.data[1];
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.cell_voll[bmu_num + 2] = can0_event.data[2];
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.cell_voll[bmu_num + 3] = can0_event.data[3];
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.cell_voll[bmu_num + 4] = can0_event.data[4];
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.cell_voll[bmu_num + 5] = can0_event.data[5];
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.cell_voll[bmu_num + 6] = can0_event.data[6];
			power_cell_t.power_ce_data_T[0].stP_b_pack_T.cell_voll[bmu_num + 7] = can0_event.data[7];
			}

		//
		return 1;
		}

	if (CanID == 0x18F813F0)
		{
		if ((can0_event.data[0] &0x0C) == 0x01)
			{
			gb_vehicle_DATA.gb_Vehicle_DATA.car_sta = Other_sta;
			}

		return 1;
		}

	///////////////////////////////////////////////////////////////////////////////i?1鈺媋嫹閿Ya枻a嫹閿Ya枻a嫹i?1i?1i?1閿Yi?1nd
	/////////////////////////////////////////////////////////////////////////////////////////閿Yai?1閿Y鐣Oi?1閿?i?1i??
	if (CanID == 0x18F813F0)
		{
		if ((can0_event.data[0] &0x0C) == 0x01)
			{
			gb_vehicle_DATA.gb_Vehicle_DATA.car_sta = Other_sta;
			}

		return 1;
		}

	/////////////////////////////////////////////////////////////////////////////////////////閿Yai?1閿Y鐣Oi?1閿?i?1a嫹end
}




int can_event0()
{
	while (can0_flg > 0) //if (can0_flg > 0)
		{
		can0_event			= can0_frame[can0_out];
		can0_data();

		if (can0_flg == 0)
			can0_flg = 0;
		else 
			can0_flg--;

		can0_out++;

		if (can0_out >= FRAME)
			can0_out = 0;
		}

	//
	Bms_Info();

	msleep(20);

	Bms_SYS_Info();

	msleep(20);

	//if (gBMSInfo.Cell_show == 1)
	//	Bms_Cell_Info();
	//msleep(300);
	//printf("------------can_event0-------------\n");
	//
	return 0;
}


//
struct can_frame can1_event;

////////////////////////////////////////////////////////

/*********************************************************************************************************
**o桬?AIc LoINFYIDAssembleTool(unsigned char ErrorCode,unsigned char EquipmentCode,unsigned char ComdCode,
	unsigned char FrameId,unsigned char ModuleAddr)
**1δ蹵eE?Lo组装觕飞源腖?镮D
**E??蜤?Lo
**调覣I酔3 Lo
********************************************************************************************************/
uint32_t INFYIDAssembleTool(unsigned char ErrorCode, unsigned char EquipmentCode, unsigned char ComdCode, 
	unsigned char TargetAddr, unsigned char SourceAddr)
{
	PowerModuleID	ID;

	ID.INFYID.bit.ErrorCode = ErrorCode;
	ID.INFYID.bit.EquipmentCode = EquipmentCode;
	ID.INFYID.bit.ComdCode = ComdCode;

	//ID.INFYID.bit.TargetAddr = TargetAddr - 1;		//ACDCzidongfenpei
	ID.INFYID.bit.TargetAddr = TargetAddr;			//ACDCzidongfenpei
	ID.INFYID.bit.SourceAddr = SourceAddr;
	ID.INFYID.bit.Rsvd	= 0;
	return ID.INFYID.all;
}


/*********************************************************************************************************
**o桬?AIc LoSetPowerModule(uint32_t ID, uint8_t Cmd,uint8_t Switch,uint16_t SetCurr,uint32_t SetVolt)
**1δ蹵eE?Lo电源腖?榈?a?O报文o桬?E礗侄缘缭吹腁渲A1a?O
**E??蜤?Lo
**调覣I酔3 Lo
********************************************************************************************************/
//
PowerModuleID	ModuleID;

//
void SetPowerModule(uint32_t ID, uint8_t Cmd, uint8_t Switch, uint32_t SetCurr, uint32_t SetVolt) //
{
	//		PowerModuleID	ModuleID;
	SetPowerModuleCurOut SetCurrent;				//腖i?1i?1i?1i?1i?1i?1i?1i?1i??	
	SetPowerModuleVoltOut SetVoltage;				//腖i?1i?1i?1i?1i?1i?1i?1N?

	struct can_frame sendINFY;

	switch (INFYPowerModule)
		{
		/*********************************************************************************************************
			**i?1i?1i?1i?1i?1i?1i?1i?1 i?1i?1觕i?1i?1源i?1i?1源腖i?1i?1?i?1O眎?1i??
			??oi?1i?1i?1,E礽?1侄缘i?1源i?1i?1i?1i?1i?1A1i?1i??				 i?1i?1i?1i?1通i?1i?
			??腖i?1i?1?1i?1i?1i?1i??			*******************************************************************
			*************************************/
		case INFYPowerModule:
			SetCurrent.IncreaseCurOut.all = SetCurr;
			SetVoltage.IncreaseVoltOut.all = SetVolt;
			ModuleID.INFYID.all = ID;

			switch (Cmd)
				{
				case INFYModule_Read: //i?1i?1i?1閿Ye奩i?1閿Ya枻a嫹a?!i?1i?1閿Yi?1
					sendINFY.can_id = ModuleID.INFYID.all;
					sendINFY.can_dlc = 8;
					sendINFY.data[0] = 0;
					sendINFY.data[1] = 0;
					sendINFY.data[2] = 0;
					sendINFY.data[3] = 0;
					sendINFY.data[4] = 0;
					sendINFY.data[5] = 0;
					sendINFY.data[6] = 0;
					sendINFY.data[7] = 0;
					can_send(1, sendINFY, 1);
					break;

				case INFYModule_Cofg: //閿Ya枻a嫹i?1i?1a嫹i?1i?1a嫹a?燦?a嫹i?1閿Ya枻i??
					switch (ModuleID.INFYID.bit.ComdCode)
						{
						case CofgModuleWalk_INFYComd: //i?1i?1i?1i?1i?1蝁i?1绛1a嫹NWALKaùi1^i?1閿Y鐣Oi?1閿Ya枻i??
						case CofgModuleLed_INFYComd: //i?1i?1i?1i?1i?1蝁i?1绛1a嫹閿Ya枻a嫹閿Y?
						case CofgModuleSysNum_INFYComd: //i?1i?1i?1i?1i?1蝁i?1绛1a嫹N?俰?1a嫹閿Y?
						case CofgSingleModuleHibernate_INFYComd: //i?1i?1i?1i?1i?1a嫹閿Y?Yi?1a嫹閿Ya枻a嫹閿Ya枻a嫹
						case CofgWholeGroupModuleOnOff_INFYComd: //閿Y鏰杋?1a嫹閿Ya枻a嫹閿Ye棄蝁i?1绛1a嫹鐎礽?1a嫹閿Ya枻a嫹i??
						case CofgModuleAdress_INFYComdTypeDistribute: //i?1i?1i?1i?1i?1蝁i?1绛1a嫹閿Ye奩i?1閿Ya枻a嫹閿Ya枻i??
							sendINFY.can_id = ModuleID.INFYID.all;
							sendINFY.can_dlc = 8;
							sendINFY.data[0] = Switch;
							sendINFY.data[1] = 0;
							sendINFY.data[2] = 0;
							sendINFY.data[3] = 0;
							sendINFY.data[4] = 0;
							sendINFY.data[5] = 0;
							sendINFY.data[6] = 0;
							sendINFY.data[7] = 0;
							can_send(1, sendINFY, 1);
							break;

						case CofgModuleReset_INFYComd:
							sendINFY.can_id = ModuleID.INFYID.all;
							sendINFY.can_dlc = 8;
							sendINFY.data[0] = 0xA5;
							sendINFY.data[1] = 0;
							sendINFY.data[2] = 0;
							sendINFY.data[3] = 0;
							sendINFY.data[4] = 0;
							sendINFY.data[5] = 0;
							sendINFY.data[6] = 0;
							sendINFY.data[7] = 0;
							can_send(1, sendINFY, 1);
							break;

						case CofgWholeGroupModuleOutPut_INFYComd: //i?1i?1i?1i?1i?1a嫹閿Ya枻a嫹a?!i?1i?1閿Ye奩i?1閿Ya枻i?1
						case CofgSingleModuleOutPut_INFYComd: //i?1i?1i?1i?1i?1a嫹閿Y?Yi?1a嫹閿Ya枻a嫹閿Ya枻a嫹
							//IrqDisable();
							sendINFY.can_id = ModuleID.INFYID.all;
							sendINFY.can_dlc = 8;
							sendINFY.data[0] = SetVoltage.IncreaseVoltOut.VoltOut.Byte.Byte3;
							sendINFY.data[1] = SetVoltage.IncreaseVoltOut.VoltOut.Byte.Byte2;
							sendINFY.data[2] = SetVoltage.IncreaseVoltOut.VoltOut.Byte.Byte1;
							sendINFY.data[3] = SetVoltage.IncreaseVoltOut.VoltOut.Byte.Byte0;
							sendINFY.data[4] = SetCurrent.IncreaseCurOut.CurOut.Byte.Byte3;
							sendINFY.data[5] = SetCurrent.IncreaseCurOut.CurOut.Byte.Byte2;
							sendINFY.data[6] = SetCurrent.IncreaseCurOut.CurOut.Byte.Byte1;
							sendINFY.data[7] = SetCurrent.IncreaseCurOut.CurOut.Byte.Byte0;
							can_send(1, sendINFY, 1);
							break;

						default:
							break;
						}

					break;
				}

			break;

		default:
			break;
		}
}


//
PowerModuleMonitorSet_BEG MonitorSet_BEG;

//
void SetPowerModule_BEG(uint32_t ID, uint8_t Cmd, uint8_t BigCmd, uint8_t SmalCmd) //
{
	//		PowerModuleID	ModuleID;
	//SetPowerModuleCurOut SetCurrent;				//腖i?1i?1i?1i?1i?1i?1i?1i?1i??	
	//SetPowerModuleVoltOut SetVoltage;				//腖i?1i?1i?1i?1i?1i?1i?1N?
	struct can_frame sendINFY;

	switch (INFYPowerModule)
		{
		/*********************************************************************************************************
			**i?1i?1i?1i?1i?1i?1i?1i?1 i?1i?1觕i?1i?1源i?1i?1源腖i?1i?1?i?1O眎?1i??
			??oi?1i?1i?1,E礽?1侄缘i?1源i?1i?1i?1i?1i?1A1i?1i??				 i?1i?1i?1i?1通i?1i?
			??腖i?1i?1?1i?1i?1i?1i??			*******************************************************************
			*************************************/
		case INFYPowerModule:
			//SetCurrent.IncreaseCurOut.all = SetCurr;
			//SetVoltage.IncreaseVoltOut.all = SetVolt;
			ModuleID.INFYID.all = ID;

			switch (Cmd)
				{
				case INFYModule_Read: //i?1i?1i?1閿Ye奩i?1閿Ya枻a嫹a?!i?1i?1閿Yi?1
					sendINFY.can_id = ModuleID.INFYID.all;
					sendINFY.can_dlc = 8;
					sendINFY.data[0] = BigCmd;
					sendINFY.data[1] = SmalCmd;
					sendINFY.data[2] = 0;
					sendINFY.data[3] = 0;
					sendINFY.data[4] = 0;
					sendINFY.data[5] = 0;
					sendINFY.data[6] = 0;
					sendINFY.data[7] = 0;
					can_send(1, sendINFY, 1);
					break;

				case INFYModule_Cofg: //閿Ya枻a嫹i?1i?1a嫹i?1i?1a嫹a?燦?a嫹i?1閿Ya枻i??
					switch (ModuleID.INFYID.bit.ComdCode)
						{
						case 0x24: //i?1i?1i?1i?1i?1蝁i?1绛1a嫹NWALKaùi1^i?1閿Y鐣Oi?1閿Ya枻i??
							switch (BigCmd)
								{
								case 0x10:
									switch (SmalCmd)
										{
										case 0x1:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.ModuleVoltOut.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.ModuleVoltOut.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.ModuleVoltOut.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.ModuleVoltOut.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x2:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.ModuleCurOut.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.ModuleCurOut.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.ModuleCurOut.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.ModuleCurOut.Array[0];
											can_send(1, sendINFY, 1);
											break;
										}

									break;

								case 0x11:
									switch (SmalCmd)
										{
										case 0x1:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.ModuleVoltOut.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.ModuleVoltOut.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.ModuleVoltOut.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.ModuleVoltOut.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x2:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.ModuleCurOut.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.ModuleCurOut.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.ModuleCurOut.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.ModuleCurOut.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x3:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = 0;
											sendINFY.data[5] = 0;
											sendINFY.data[6] = MonitorSet_BEG.Module_Num.Bytes[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_Num.Bytes[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x10:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = 0;
											sendINFY.data[5] = 0;
											sendINFY.data[6] = MonitorSet_BEG.Module_Run.Bytes[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_Run.Bytes[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x21:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = 0;
											sendINFY.data[5] = 0;
											sendINFY.data[6] = MonitorSet_BEG.Module_sleep.Bytes[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_sleep.Bytes[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x22:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = 0;
											sendINFY.data[5] = 0;
											sendINFY.data[6] = MonitorSet_BEG.Module_Walkin.Bytes[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_Walkin.Bytes[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x23:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = 0;
											sendINFY.data[5] = 0;
											sendINFY.data[6] = MonitorSet_BEG.Module_Add.Bytes[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_Add.Bytes[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x32:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_DC_minVolt.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_DC_minVolt.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_DC_minVolt.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_DC_minVolt.Array[0];
											can_send(1, sendINFY, 1);
											break;
										}

									break;

								case 0x21:
									switch (SmalCmd)
										{
										case 0x1:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_Volt.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_Volt.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_Volt.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_Volt.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x2:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_HZ.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_HZ.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_HZ.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_HZ.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x5:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_PF.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_PF.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_PF.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_PF.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x8:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_LP.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_LP.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_LP.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_LP.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x10:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = 0;
											sendINFY.data[5] = 0;
											sendINFY.data[6] = MonitorSet_BEG.Module_Word.Bytes[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_Word.Bytes[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x11:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = 0;
											sendINFY.data[5] = 0;
											sendINFY.data[6] = MonitorSet_BEG.Module_3P.Bytes[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_3P.Bytes[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x14:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = 0;
											sendINFY.data[5] = 0;
											sendINFY.data[6] = MonitorSet_BEG.Module_Only.Bytes[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_Only.Bytes[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x17:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = 0;
											sendINFY.data[5] = 0;
											sendINFY.data[6] = MonitorSet_BEG.Module_LPSer.Bytes[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_LPSer.Bytes[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x20:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_OV1.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_OV1.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_OV1.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_OV1.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x21:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_OV1_T.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_OV1_T.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_OV1_T.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_OV1_T.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x22:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_OV2.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_OV2.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_OV2.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_OV2.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x23:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_OV2_T.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_OV2_T.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_OV2_T.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_OV2_T.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x24:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_UV1.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_UV1.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_UV1.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_UV1.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x25:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_UV1_T.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_UV1_T.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_UV1_T.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_UV1_T.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x26:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_UV2.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_UV2.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_UV2.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_UV2.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x27:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_UV2_T.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_UV2_T.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_UV2_T.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_UV2_T.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x28:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_HZ_OV1.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_HZ_OV1.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_HZ_OV1.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_HZ_OV1.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x29:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_HZ_OV1_T.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_HZ_OV1_T.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_HZ_OV1_T.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_HZ_OV1_T.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x2A:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_HZ_OV2.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_HZ_OV2.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_HZ_OV2.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_HZ_OV2.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x2B:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_HZ_OV2_T.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_HZ_OV2_T.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_HZ_OV2_T.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_HZ_OV2_T.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x2C:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_HZ_UV1.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_HZ_UV1.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_HZ_UV1.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_HZ_UV1.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x2D:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_HZ_UV1_T.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_HZ_UV1_T.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_HZ_UV1_T.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_HZ_UV1_T.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x2E:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_HZ_UV2.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_HZ_UV2.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_HZ_UV2.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_HZ_UV2.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x2F:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_HZ_UV2_T.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_HZ_UV2_T.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_HZ_UV2_T.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_HZ_UV2_T.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x30:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_OV.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_OV.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_OV.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_OV.Array[0];
											can_send(1, sendINFY, 1);
											break;

										case 0x31:
											sendINFY.can_id = ModuleID.INFYID.all;
											sendINFY.can_dlc = 8;
											sendINFY.data[0] = BigCmd;
											sendINFY.data[1] = SmalCmd;
											sendINFY.data[2] = 0;
											sendINFY.data[3] = 0;
											sendINFY.data[4] = MonitorSet_BEG.Module_AC_OV_T.Array[3];
											sendINFY.data[5] = MonitorSet_BEG.Module_AC_OV_T.Array[2];
											sendINFY.data[6] = MonitorSet_BEG.Module_AC_OV_T.Array[1];
											sendINFY.data[7] = MonitorSet_BEG.Module_AC_OV_T.Array[0];
											can_send(1, sendINFY, 1);
											break;
										}

									break;
								}

							break;

						default:
							break;
						}

					break;
				}

			break;

		default:
			break;
		}
}


//
enum 
{
	CheckCANInit = 0,								//CAN3oE1粭自1i
	Boost,											//升N1
	BoostAndReadBack,								//升N1o突O读
	ReductionVoltAndReadBack,						//1礜1o突O读
	AgainstImpactCurr,								//反3a击电流
	DCModuleControlInit,							//腖??O諥3oE1粭
	DCModuleReadBack,								//腖?榛O读
	TappingAndReadBack, 							//?1稟与籓读
	ScanModuleCommu,								//扫Ae腖?橥?A
	CofgModuleReset
};


//
#define InsulationDetectsBoostCurr 1000L//mA  i?1i?1缘i?1i?1i?1i?1i?1N1i?1i?1i?1i??//
#define VINCodeSize 			17				 //Vin码3ざE
#define BMSSoftVersionSize		8
typedef struct 
{
	uint32_t		ChgCycle;						//电3O3涞绱蜤?
	uint16_t		Cell_MaxAllowChargeVolt;		//礩Ia稐力?畹?O最高訣??涞绲鏝1:E?莘直a翬Lo0.01VL椢籐?VA玂A?L籈?莘段o0V-24V;
	uint16_t		MaxAllowChargeCurr; 			//最高訣??涞绲缌?E?莘直a翬Lo0.1AL椢籐琌?00AA玂A?L籈?莘段oO?00A-0A;
	uint16_t		NomTotalCapacity;				//稐力?畹?O眅3A总能?:E?莘直a翬Lo0.1kW!/位L?kW!A玂A?L籈?莘段o0-1000kW!;
	uint16_t		MaxAllowChgTotalVolt;			//最高訣??涞缱艿鏝1:E?莘直a翬Lo0.1VL椢籐?VA玂A?L籈?莘段o0V-750V;
	uint16_t		SOC;							//Ou3刀椓?畹?Oo傻缱碔琇⊿OC)LoE?莘直a翬Lo0.1%L椢籐?%A玂A?L籈?莘段o0-100%;
	uint16_t		TotalVolt;						//Ou3刀椓?畹?O总电N1LoE?莘直a翬Lo0.1VL椢籐?VA玂A?L籈?莘段o0V-750V.
	uint16_t		BatRatedCapacity;				//Ou3刀椓?畹?OI低3额定E萘?/A.h,0.1A/bit,0A.hA玂A?L珽?莘段o0-1000A.h
	uint16_t		BatRatedTotalVolt;				//Ou3刀椓?畹?OI低3额定总电N10.1V/bit,0VA玂A?L珽?莘段o0-750V
	uint8_t 		BatManufacName[4];				//电3O生2IAu3ALe?ASCII码
	uint8_t 		BatPackNum[4];					//电3O组?ooA
	uint8_t 		BMSSoftVersion[BMSSoftVersionSize]; //BMSE??癮?oA 
	uint8_t 		VIN[VINCodeSize];				//3盗3E侗?码
	uint8_t 		BatManfacDate[3];				//电3O生2鶨OA贚o膃月EO
	uint8_t 		BatType;						//电3O?蚾A:01HLo铅酸电3OL?2HLo膐氢电3OL?3HLo磷酸I鷌?OL?4H:AI酸i?OL?5H:钴酸i?OL?6HLoEa2牧I电3OL?7HLo3UoI蝘i瓵胱拥?OL?8HLo頝酸i?OL籉FHLoA渌u电3O
	uint8_t 		MaxAllowTemp;					//最高訣?矶椓?畹?O温禘:E?莘直a翬Lo1!aL椢籐琌?0!aA玂A?L籈?莘段oO?0!aO籐?00!a
	uint8_t 		PropertyMark;
} BATTERYPARAM;


BATTERYPARAM	BatteryParam;



////////////////////
#define CurrOffset				32000			   //i?1i?1i?1i?1A玦?1i?1i?1i?1
#define TempOffset				50				   //i?1露i?1A玦?1i?1i?1i?1

/*********************************************************************************************************
**o桬?AIcLovoid (CAN_MSG_Type	Message,PowerModuleMonitorAck MonitorAck,uint8_t *AckState)
**1δ蹵eE?Lo觕?蒃?电源腖?榈幕O复报文1馕?**E銭?蜤?LoCAN_MSG_Type	Message
**E??蜤?Lo
**调覣I酔3 Lo
********************************************************************************************************/
//
PowerModuleMonitorAck MonitorAck[11]; //max:10+1
PowerModuleMonitorAck_BEG MonitorAck_BEG[11]; //max:10+1
PowerModuleMonitorAck_WL gMonitorPG[2];

//
WL_Info 		gWL_Info;
DEG_Info		gDEG_Info;

//
uint8_t 		gAckState;

//
void PowerModuleAckMsg(struct can_frame * Message, PowerModuleMonitorAck MonitorAck[DCModeNum], 
	uint8_t * AckState, 
	uint8_t PowerModuleType, uint8_t PowerDistribution) //觕i?1i?1i?1i?1i?1源i?1幕O竔?1i?1i?1i?1i??
{
	//		PowerModuleID	ModuleID;
	LongWordByte	OutValue;
	ByteBit 		Status;
	uint8_t 		MsgStatus, Index = 0;

	switch (PowerModuleType)
		{
		/*********************************************************************************************************
		**i?1i?1i?1i?1i?1i?1i?1i?1 i?1i?1觕i?1i?1i?1i?1i?1源腖i?1i?1幕O竔?1i?1i?1?
			?1?i?1i?1i?1
		********************************************************************************************************/
		case IncreasePowerModule:
			break;

		/*********************************************************************************************************
		**i?1i?1i?1i?1i?1i?1i?1i?1 i?1i?1i?1i?1i?1i?1i?1i?1O礽?1源腖i?1i?1幕O竔?1?
			?1i?1i?1?i?1i?1i?1
		********************************************************************************************************/
		case MegmeetPowerModule:
			break;

		/*********************************************************************************************************
		**i?1i?1i?1i?1i?1i?1i?1i?1 i?1i?1i?1i?1蝍i?1i?1源腖i?1i?1幕O竔?1i?1i?1i?1?
			?i?1i?1i??		******************************************************************************************
			**************/
		case HuaWeiPowerModule:
			break;

		/*********************************************************************************************************
		**i?1i?1i?1i?1i?1i?1i?1i?1 i?1i?1i?1i?1?i?1i?1i?1i?1源腖i?1i?1幕O竔?1i?1?
			?1i?1?i?1i?1i??		*********************************************************************************
			***********************/
		case GaoSiBaoPowerModule:
			break;

		/*********************************************************************************************************
		**i?1i?1i?1i?1i?1i?1i?1i?1 i?1i?1通i?1I礽?1源腖i?1i?1幕O竔?1i?1i?1i?1?i??
			??1i??		**********************************************************************************************
			**
			********/
		case TongHePowerModule:
			break;

		/*********************************************************************************************************
		**i?1i?1i?1i?1i?1i?1i?1i?1 i?1i?1觕i?1i?1源i?1i?1源腖i?1i?1幕O竔?1i?1i?1i?
			??i?1i?1i??		***************************************************************************************
			*****************/
		case INFYPowerModule:
			ModuleID.INFYID.all = Message->can_id;

			if (ModuleID.INFYID.bit.ErrorCode == Normal &&
				 (ModuleID.INFYID.bit.EquipmentCode == SingleModuleProtocol ||
				 ModuleID.INFYID.bit.EquipmentCode == WholeGroupModuleProtocol) &&
				 (ModuleID.INFYID.bit.TargetAddr == MonitoringAdress ||
				 ModuleID.INFYID.bit.TargetAddr == BroadcastAddress))
				{
				Index				= ModuleID.INFYID.bit.SourceAddr + 1; // ACDCzidongfenpei 0

				if (Index && (Index < (DCModeNum + 1)))
					{
					Index--;

					switch (ModuleID.INFYID.bit.ComdCode)
						{
						case ReadModuleStatus_INFYComd: //i?1i?1E!腖i?1i?1状I?							MonitorAck[Index].Temp = Message->data[4] +TempOffset;
							MonitorAck[Index].Err.INFYBit.StateTable2.all = Message->data[5];
							MonitorAck[Index].Err.INFYBit.StateTable1.all = Message->data[6];
							MonitorAck[Index].Err.INFYBit.StateTable0.all = Message->data[7];
							MonitorAck[Index].Err.bit.RxMessageFlag = 1;
							MonitorAck[Index].Err.bit.ComuQuality = 1;

							//EqupeDiag.Comu.DCModeRxFlag = 1;
							break;

						case ReadSingleModuleOutPut_INFYComd: //i?1i?1E!i?1i?1腖i?1i?1i?1N1i?1i?1i?1i??							
							OutValue.Byte.Byte0 = Message->data[3];
							OutValue.Byte.Byte1 = Message->data[2];
							OutValue.Byte.Byte2 = Message->data[1];
							OutValue.Byte.Byte3 = Message->data[0];
							MonitorAck[Index].ModuleVoltOut.all = (uint16_t) (OutValue.all / 100L);
							OutValue.Byte.Byte0 = Message->data[7];
							OutValue.Byte.Byte1 = Message->data[6];
							OutValue.Byte.Byte2 = Message->data[5];
							OutValue.Byte.Byte3 = Message->data[4];
							MonitorAck[Index].ModuleCurOut.all = (uint16_t) (OutValue.all / 100L);
							MonitorAck[Index].Err.bit.RxMessageFlag = 1;
							MonitorAck[Index].Err.bit.ComuQuality = 1;

							//EqupeDiag.Comu.DCModeRxFlag = 1;
							break;

						case CofgWholeGroupModuleOutPut_INFYComd: //i?1i?1i?1i?1i?1i?1i?1i?1腖i?1i?1i?1i?1i??			
						case CofgSingleModuleOutPut_INFYComd: //i?1i?1i?1A礽?1腖i?1i?1i?1i?1i??						
						case CofgWholeGroupModuleOnOff_INFYComd: //i?1i?1i?1i?1i?1i?1i?1i?1腖i?1?ai?1O籭?1
						case CofgModuleReset_INFYComd:
							MonitorAck[Index].Err.bit.RxMessageFlag = 1;
							MonitorAck[Index].Err.bit.ComuQuality = 1;

							//EqupeDiag.Comu.DCModeRxFlag = 1;
							break;

						default:
							break;
						}
					}
				}

			break;

		default:
			break;
		}

	switch (PowerDistribution)
		{
		case TurnlistCharge:
			break;

		case SimultanouslyCharge:
			break;

		default:
			break;
		}

}


////////////////////
//
void PowerModuleAckMsg_BEG(struct can_frame * Message, 
	uint8_t * AckState, 
	uint8_t PowerModuleType, uint8_t PowerDistribution) //觕i?1i?1i?1i?1i?1源i?1幕O竔?1i?1i?1i?1i??
{
	//		PowerModuleID	ModuleID;
	LongWordByte	OutValue;
	ByteBit 		Status;
	uint8_t 		MsgStatus, Index = 0;


	ModuleID.INFYID.all = Message->can_id;

	if (ModuleID.INFYID.bit.ErrorCode == Normal && (ModuleID.INFYID.bit.EquipmentCode == SingleModuleProtocol ||
		 ModuleID.INFYID.bit.EquipmentCode == WholeGroupModuleProtocol) &&
		 (ModuleID.INFYID.bit.TargetAddr == MonitoringAdress || ModuleID.INFYID.bit.TargetAddr == BroadcastAddress))
		{
		Index				= ModuleID.INFYID.bit.SourceAddr;

		//
		if (Index == 0x3F) //1a2Y
			{
			switch (ModuleID.INFYID.bit.ComdCode)
				{
				case 0x23: //读
					switch (Message->data[0])
						{
						case 0x10:
							switch (Message->data[1])
								{
								case 0x1:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[0].ModuleVoltOut.all = (uint32_t) (OutValue.all / 100L);
									break;

								case 0x2:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[0].ModuleCurOut.all = (uint16_t) (OutValue.all / 100L);
									break;
								}

							break;
						}
				}

			return;
			}

		//
		//Index				= Index				-1;
		//
		//if (Index && (Index < (DCModeNum + 1)))
		if (1)
			{
			//Index--;
			switch (ModuleID.INFYID.bit.ComdCode)
				{
				case 0x23: //?
					switch (Message->data[0])
						{
						case 0x10:
							switch (Message->data[1])
								{
								case 0x1:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];

									//MonitorAck_BEG[Index].ModuleVoltOut.all = (uint32_t) (OutValue.all / 100L);
									break;

								case 0x2:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];

									//MonitorAck_BEG[Index].ModuleCurOut.all = (uint16_t) (OutValue.all / 100L);
									break;
								}

							break;

						case 0x11:
							switch (Message->data[1])
								{
								case 0x1:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].ModuleVoltOut.all = (uint32_t) (OutValue.all / 100L);
									break;

								case 0x2:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].ModuleCurOut.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x3:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_ABVolt.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x4:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_BCVolt.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x5:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_CAVolt.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x6:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Temp = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x10:
									MonitorAck_BEG[Index].Err.INFYBit.StateTable2.all = Message->data[5];
									MonitorAck_BEG[Index].Err.INFYBit.StateTable1.all = Message->data[6];
									MonitorAck_BEG[Index].Err.INFYBit.StateTable0.all = Message->data[7];
									MonitorAck_BEG[Index].Err.bit.RxMessageFlag = 1;
									MonitorAck_BEG[Index].Err.bit.ComuQuality = 1;
									break;

								case 0x11:
									MonitorAck_BEG[Index].Err_AC.INFYBit.StateTable2.all = Message->data[5];
									MonitorAck_BEG[Index].Err_AC.INFYBit.StateTable1.all = Message->data[6];
									MonitorAck_BEG[Index].Err_AC.INFYBit.StateTable0.all = Message->data[7];
									MonitorAck_BEG[Index].Err_AC.bit.RxMessageFlag = 1;
									MonitorAck_BEG[Index].Err_AC.bit.ComuQuality = 1;
									break;
								}

							break;

						case 0x21:
							switch (Message->data[1])
								{
								case 0x1:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_AVolt.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x2:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_BVolt.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x3:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_CVolt.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x4:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_ACur.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x5:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_BCur.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x6:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_CCur.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x7:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_HZ.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x8:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_AllP.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x9:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_APower.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0xA:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_BPower.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0xB:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_CPower.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0xC:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_AllLP.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0xD:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_LAPower.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0xE:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_LBPower.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0xF:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].Module_AC_LCPower.all = (uint16_t) (OutValue.all / 100L);
									break;
								}

							break;
						}

					//EqupeDiag.Comu.DCModeRxFlag = 1;
					break;

				case 0x24: //蒭	1a2Y?籓复		
					switch (Message->data[0])
						{
						case 0x10:
							switch (Message->data[1])
								{
								case 0x1:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].ModuleVoltOut.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x2:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].ModuleCurOut.all = (uint16_t) (OutValue.all / 100L);
									break;
								}

							break;

						case 0x11:
							switch (Message->data[1])
								{
								case 0x1:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].ModuleVoltOut.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x2:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].ModuleCurOut.all = (uint16_t) (OutValue.all / 100L);
									break;
								}

							break;

						case 0x21:
							switch (Message->data[1])
								{
								case 0x1:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].ModuleVoltOut.all = (uint16_t) (OutValue.all / 100L);
									break;

								case 0x2:
									OutValue.Byte.Byte0 = Message->data[7];
									OutValue.Byte.Byte1 = Message->data[6];
									OutValue.Byte.Byte2 = Message->data[5];
									OutValue.Byte.Byte3 = Message->data[4];
									MonitorAck_BEG[Index].ModuleCurOut.all = (uint16_t) (OutValue.all / 100L);
									break;
								}

							break;
						}

					//EqupeDiag.Comu.DCModeRxFlag = 1;
					break;

				default:
					break;
				}
			}
		}
}


////////////////////
enum 
{
	TurnOn = 0, 
	TurnOff
};


////////////////////
//腖i?1i?1i?1i?1i?1i?1i?1i?1i??
void ModuleOutPutControTool(uint32_t Volt, uint32_t Curr, uint8_t Adress, uint8_t PowerModuleType, uint8_t OnOff)
{
	switch (OnOff)
		{
		case TurnOn:
			switch (PowerModuleType)
				{
				case IncreasePowerModule:
					break;

				case MegmeetPowerModule:
					break;

				case HuaWeiPowerModule:
					break;

				case GaoSiBaoPowerModule:
					break;

				case TongHePowerModule:
					break;

				case INFYPowerModule:
					SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, CofgSingleModuleOutPut_INFYComd, Adress + 1, MonitoringAdress),
						 
						INFYModule_Cofg, 0, Curr, Volt);
					msleep(20);
					SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, CofgWholeGroupModuleOnOff_INFYComd, Adress + 1, MonitoringAdress),
						 
						INFYModule_Cofg, INFYDCModuleOn, 0, 0); //i?1i?1DCMode
					msleep(20);
					break;

				default:
					break;
				}

			break;

		case TurnOff:
			switch (PowerModuleType)
				{
				case IncreasePowerModule:
					break;

				case MegmeetPowerModule:
					break;

				case HuaWeiPowerModule:
					break;

				case GaoSiBaoPowerModule:
					break;

				case TongHePowerModule:
					break;

				case INFYPowerModule:
					SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, CofgWholeGroupModuleOnOff_INFYComd, Adress + 1, MonitoringAdress),
						 
						INFYModule_Cofg, INFYDCModuleOff, 0, 0); //i?1i?1DCMode
					msleep(20);
					break;

				default:
					break;
				}

			break;
		}
}


////////////////////
////////////////////
////////////////////
void ModuleOutPutControl(uint8_t Choice, uint8_t ModuleAdress)
{
	PowerModuleID	DCModuleID;
	uint32_t		OutPutVolt = 0;
	uint8_t 		i	= 0;

	switch (Choice)
		{
		case CheckCANInit: //CANi?1i?1E1i?1i?1i?1?i?1
			switch (INFYPowerModule)
				{
				case IncreasePowerModule:
					break;

				case MegmeetPowerModule:
					break;

				case HuaWeiPowerModule:
					break;

				case GaoSiBaoPowerModule:
					break;

				case TongHePowerModule:
					break;

				case INFYPowerModule:
					INFYDCModule_OFF;
					msleep(20);
					SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, CofgModuleAdress_INFYComdTypeDistribute, BroadcastAddress, MonitoringAdress),
						 
						INFYModule_Cofg, 1, 0, 0);
					msleep(20);

					for (i = 0; i < DCModeNum - 1; i++)
						{
						SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, ReadModuleStatus_INFYComd, i + 1, MonitoringAdress),
							 
							INFYModule_Read, 0, 0, 0);
						msleep(20);
						}

					break;

				default:
					break;
				}

			break;

		case Boost: //i?1i?1N1
			switch (INFYPowerModule)
				{
				case IncreasePowerModule:
					break;

				case MegmeetPowerModule:
					break;

				case HuaWeiPowerModule:
					break;

				case GaoSiBaoPowerModule:
					break;

				case TongHePowerModule:
					break;

				case INFYPowerModule:
					SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, CofgSingleModuleOutPut_INFYComd, ModuleAdress, MonitoringAdress),
						 
						INFYModule_Cofg, 0, InsulationDetectsBoostCurr, 
						(uint32_t) BatteryParam.MaxAllowChgTotalVolt * 100L);
					msleep(20);
					SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, CofgWholeGroupModuleOnOff_INFYComd, ModuleAdress, MonitoringAdress),
						 
						INFYModule_Cofg, INFYDCModuleOn, 0, 0); //鐎礽?1a嫹CMode
					msleep(20);
					break;

				default:
					break;
				}

			break;

		case BoostAndReadBack: //閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹i?1i?1a嫹
			switch (INFYPowerModule)
				{
				case IncreasePowerModule:
					break;

				case MegmeetPowerModule:
					break;

				case HuaWeiPowerModule:
					break;

				case GaoSiBaoPowerModule:
					break;

				case TongHePowerModule:
					break;

				case INFYPowerModule:
					SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, CofgSingleModuleOutPut_INFYComd, ModuleAdress, MonitoringAdress),
						 
						INFYModule_Cofg, 0, InsulationDetectsBoostCurr, 
						(uint32_t) BatteryParam.MaxAllowChgTotalVolt * 100L);
					msleep(20);
					SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, CofgWholeGroupModuleOnOff_INFYComd, ModuleAdress, MonitoringAdress),
						 
						INFYModule_Cofg, INFYDCModuleOn, 0, 0);
					SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, ReadSingleModuleOutPut_INFYComd, ModuleAdress, MonitoringAdress),
						 
						INFYModule_Read, 0, 0, 0);
					msleep(20);
					break;

				default:
					break;
				}

			break;

		case ReductionVoltAndReadBack: //閿Ya枻a嫹閿Ya枻a嫹閿Ya枻a嫹i?1i?1a嫹
			switch (INFYPowerModule)
				{
				case IncreasePowerModule:
					break;

				case MegmeetPowerModule:
					break;

				case HuaWeiPowerModule:
					break;

				case GaoSiBaoPowerModule:
					break;

				case TongHePowerModule:
					break;

				case INFYPowerModule:
					INFYDCModule_OFF;
					SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, ReadSingleModuleOutPut_INFYComd, ModuleAdress, MonitoringAdress),
						 
						INFYModule_Read, 0, 0, 0);
					msleep(20);
					break;

				default:
					break;
				}

			break;

		case AgainstImpactCurr: //閿Ya枻a嫹閿Ya枻a嫹i?1a?a嫹a?瀍33a嫹
			switch (INFYPowerModule)
				{
				case IncreasePowerModule:
					break;

				case MegmeetPowerModule:
					break;

				case HuaWeiPowerModule:
					break;

				case GaoSiBaoPowerModule:
					break;

				case TongHePowerModule:
					break;

				case INFYPowerModule:
					SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, CofgSingleModuleOutPut_INFYComd, BroadcastAddress, MonitoringAdress),
						 
						INFYModule_Cofg, 0, 2000L, (uint32_t) BatteryParam.TotalVolt * 100L);
					INFYDCModule_ON;
					msleep(20);

					switch (TurnlistCharge)
						{
						case TurnlistCharge:
							//
							for (i = 0; i < DCModeNum - 1; i++)
								{
								SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, ReadSingleModuleOutPut_INFYComd, i + 1, MonitoringAdress),
									 
									INFYModule_Read, 0, 0, 0);
								msleep(20);

								}

							break;

						case SimultanouslyCharge:
							break;

						default:
							break;
						}

					break;

				default:
					break;
				}

			break;

		case DCModuleControlInit: //a?!i?1i?1閿Ya枻a嫹i?1閿Ya枻a嫹閿Ya枻a嫹閿Ya枻i?1
			break;

		case DCModuleReadBack: //a?!i?1i?1閿Ya枻a嫹i?1閿Yi??
			switch (INFYPowerModule)
				{
				case IncreasePowerModule:
					break;

				case MegmeetPowerModule:
					break;

				case HuaWeiPowerModule:
					break;

				case GaoSiBaoPowerModule:
					break;

				case TongHePowerModule:
					break;

				case INFYPowerModule:
					SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, ReadModuleStatus_INFYComd, ModuleAdress, MonitoringAdress),
						 
						INFYModule_Read, 0, 0, 0);
					msleep(20);
					SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, ReadSingleModuleOutPut_INFYComd, ModuleAdress, MonitoringAdress),
						 
						INFYModule_Read, 0, 0, 0);

					if (0)
						{
						msleep(20);
						SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, CofgModuleAdress_INFYComdTypeDistribute, BroadcastAddress, MonitoringAdress),
							 
							INFYModule_Cofg, 1, 0, 0);
						}

					break;

				default:
					break;
				}

			break;

		case TappingAndReadBack: //a?瀒?1a嫹閿Ye奩i?1閿Ya枻a嫹i?1i?1a嫹
			switch (INFYPowerModule)
				{
				case IncreasePowerModule:
					break;

				case MegmeetPowerModule:
					break;

				case HuaWeiPowerModule:
					break;

				case GaoSiBaoPowerModule:
					break;

				case TongHePowerModule:
					break;

				case INFYPowerModule:
					//BatChgState.OutPutCurr = 0;
					INFYDCModule_OFF;

					switch (TurnlistCharge)
						{
						case TurnlistCharge:
							sleep(1);

							for (i = 0; i < DCModeNum - 1; i++)
								{
								SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, ReadSingleModuleOutPut_INFYComd, i + 1, MonitoringAdress),
									 
									INFYModule_Read, 0, 0, 0);

								//BatChgState.OutPutCurr += PowerModuleAck[LeftPile][i].ModuleCurOut.all;
								//OutPutVolt			+= PowerModuleAck[LeftPile][i].ModuleVoltOut.all;
								msleep(20);
								INFYDCModule_OFF;

								//msleep(20);
								}

							break;

						case SimultanouslyCharge:
							break;

						default:
							break;
						}

					break;

				default:
					break;
				}

			//BatChgState.OutPutVolt = (uint16_t)(OutPutVolt / WorkData[LeftPile].DCModeStatisticsNum);
			break;

		case ScanModuleCommu:
			switch (INFYPowerModule)
				{
				case IncreasePowerModule:
					break;

				case MegmeetPowerModule:
					break;

				case HuaWeiPowerModule:
					break;

				case GaoSiBaoPowerModule:
					break;

				case TongHePowerModule:
					break;

				case INFYPowerModule:
					for (i = 0; i < DCModeNum - 1; i++)
						{
						SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, ReadModuleStatus_INFYComd, i + 1, MonitoringAdress),
							 
							INFYModule_Read, 0, 0, 0); //閿Ya枻a嫹閿Y?
						msleep(20);
						}

					break;

				default:
					break;
				}

			break;

		case CofgModuleReset:
			switch (INFYPowerModule)
				{
				case IncreasePowerModule:
					break;

				case MegmeetPowerModule:
					break;

				case HuaWeiPowerModule:
					break;

				case GaoSiBaoPowerModule:
					break;

				case TongHePowerModule:
					break;

				case INFYPowerModule:
					msleep(20);

					for (i = 0; i < 3; i++)
						{
						SetPowerModule(INFYIDAssembleTool(Normal, SingleModuleProtocol, CofgModuleReset_INFYComd, BroadcastAddress, MonitoringAdress),
							 
							INFYModule_Cofg, 0, 0, 0); //閿Ya枻a嫹閿Y?						
						msleep(20);
						}

					break;

				default:
					break;
				}

			break;

		default:
			break;
		}
}


//
void gMonitorAck_BEG(void)
{
	unsigned char	i;

	for (i = 0; i < (DCModeNum); i++) //
		{

		printf("Err%d: %d \n", i, MonitorAck_BEG[i].Err.all);
		printf("Err_AC%d: %d \n", i, MonitorAck_BEG[i].Err_AC.all);
		printf("Module_AC_AVolt%d: %d \n", i, MonitorAck_BEG[i].Module_AC_AVolt.all);
		printf("Module_AC_BVolt%d: %d \n", i, MonitorAck_BEG[i].Module_AC_BVolt.all);
		printf("Module_AC_CVolt%d: %d \n", i, MonitorAck_BEG[i].Module_AC_CVolt.all);
		printf("Module_AC_ABVolt%d: %d \n", i, MonitorAck_BEG[i].Module_AC_ABVolt.all);
		printf("Module_AC_BCVolt%d: %d \n", i, MonitorAck_BEG[i].Module_AC_BCVolt.all);
		printf("Module_AC_CAVolt%d: %d \n", i, MonitorAck_BEG[i].Module_AC_CAVolt.all);
		printf("Module_AC_ACur%d: %d \n", i, MonitorAck_BEG[i].Module_AC_ACur.all);
		printf("Module_AC_BCur%d: %d \n", i, MonitorAck_BEG[i].Module_AC_BCur.all);
		printf("Module_AC_CCur%d: %d \n", i, MonitorAck_BEG[i].Module_AC_CCur.all);
		printf("Module_AC_HZ%d: %d \n", i, MonitorAck_BEG[i].Module_AC_HZ.all);
		printf("Module_AC_AllP%d: %d \n", i, MonitorAck_BEG[i].Module_AC_AllP.all);
		printf("Module_AC_APower%d: %d \n", i, MonitorAck_BEG[i].Module_AC_APower.all);
		printf("Module_AC_BPower%d: %d \n", i, MonitorAck_BEG[i].Module_AC_BPower.all);
		printf("Module_AC_CPower%d: %d \n", i, MonitorAck_BEG[i].Module_AC_CPower.all);
		printf("Module_AC_AllLP%d: %d \n", i, MonitorAck_BEG[i].Module_AC_AllLP.all);
		printf("Module_AC_LAPower%d: %d \n", i, MonitorAck_BEG[i].Module_AC_LAPower.all);
		printf("Module_AC_LBPower%d: %d \n", i, MonitorAck_BEG[i].Module_AC_LBPower.all);
		printf("Module_AC_LCPower%d: %d \n", i, MonitorAck_BEG[i].Module_AC_LCPower.all);
		printf("Temp%d: %d \n", i, MonitorAck_BEG[i].Temp);
		}
}


//
//
void gPrintf(void)
{
	printf("ModuleCurOut: %d \n", MonitorSet_BEG.ModuleCurOut.all);
	printf("ModuleVoltOut: %d \n", MonitorSet_BEG.ModuleVoltOut.all);
	printf("Module_Num: %d \n", MonitorSet_BEG.Module_Num.all);
	printf("Module_Run: %d \n", MonitorSet_BEG.Module_Run.all);
	printf("Module_sleep: %d \n", MonitorSet_BEG.Module_sleep.all);
	printf("Module_Walkin: %d \n", MonitorSet_BEG.Module_Walkin.all);
	printf("Module_Add: %d \n", MonitorSet_BEG.Module_Add.all);
	printf("Module_DC_minVolt: %d \n", MonitorSet_BEG.Module_DC_minVolt.all);
	printf("Module_AC_HZ: %d \n", MonitorSet_BEG.Module_AC_HZ.all);
	printf("Module_AC_Volt: %d \n", MonitorSet_BEG.Module_AC_Volt.all);
	printf("Module_AC_PF: %d \n", MonitorSet_BEG.Module_AC_PF.all);
	printf("Module_AC_LP: %d \n", MonitorSet_BEG.Module_AC_LP.all);
	printf("Module_Word: %d \n", MonitorSet_BEG.Module_Word.all);
	printf("Module_3P: %d \n", MonitorSet_BEG.Module_3P.all);
	printf("Module_Only: %d \n", MonitorSet_BEG.Module_Only.all);
	printf("Module_LPSer: %d \n", MonitorSet_BEG.Module_LPSer.all);
	printf("Module_AC_OV1: %d \n", MonitorSet_BEG.Module_AC_OV1.all);
	printf("Module_AC_OV1_T: %d \n", MonitorSet_BEG.Module_AC_OV1_T.all);
	printf("Module_AC_OV2: %d \n", MonitorSet_BEG.Module_AC_OV2.all);
	printf("Module_AC_OV2_T: %d \n", MonitorSet_BEG.Module_AC_OV2_T.all);
	printf("Module_AC_UV1: %d \n", MonitorSet_BEG.Module_AC_UV1.all);
	printf("Module_AC_UV1_T: %d \n", MonitorSet_BEG.Module_AC_UV1_T.all);
	printf("Module_AC_UV2: %d \n", MonitorSet_BEG.Module_AC_UV2.all);
	printf("Module_AC_UV2_T: %d \n", MonitorSet_BEG.Module_AC_UV2_T.all);
	printf("Module_HZ_OV1: %d \n", MonitorSet_BEG.Module_HZ_OV1.all);
	printf("Module_HZ_OV1_T: %d \n", MonitorSet_BEG.Module_HZ_OV1_T.all);
	printf("Module_HZ_OV2: %d \n", MonitorSet_BEG.Module_HZ_OV2.all);
	printf("Module_HZ_OV2_T: %d \n", MonitorSet_BEG.Module_HZ_OV2_T.all);
	printf("Module_HZ_UV1: %d \n", MonitorSet_BEG.Module_HZ_UV1.all);
	printf("Module_HZ_UV1_T: %d \n", MonitorSet_BEG.Module_HZ_UV1_T.all);
	printf("Module_HZ_UV2: %d \n", MonitorSet_BEG.Module_HZ_UV2.all);
	printf("Module_HZ_UV2_T: %d \n", MonitorSet_BEG.Module_HZ_UV2_T.all);
	printf("Module_AC_OV: %d \n", MonitorSet_BEG.Module_AC_OV.all);
	printf("Module_AC_OV_T: %d \n", MonitorSet_BEG.Module_AC_OV_T.all);
}


////////////////////////////////////////////////////////
void can1_data()
{
	unsigned long	CanID;

	//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	/*if (can1_event.can_id & CAN_EFF_FLAG)
		{
		CanID				= can1_event.can_id & CAN_EFF_MASK;
		can1_event.can_id	= CanID;
		}
	else 
		{
		CanID				= can1_event.can_id & CAN_SFF_MASK;
		can1_event.can_id	= CanID;
		}
	*/
	//CanID				= can1_event.can_id;
	/////////////////////////////////////////////////////////////////////////////////////////?????????????????

	/*
		switch (CanID)
			{
			case 0:
				usr_data_bwc_un.stusr_data_bwc.bwc_door1 = 0x02;
				break;

			case 0xfe:
				usr_data_bwc_un.stusr_data_bwc.bwc_door1 = 0xfe;
				break;

			default:
				usr_data_bwc_un.stusr_data_bwc.bwc_door1 = 0x01;
				break;
			}
	*/
	/////////////////
	//PowerModuleAckMsg(&can1_event, PowerModuleAck, &gAckState, INFYPowerModule, TurnlistCharge);
	PowerModuleAckMsg_BEG(&can1_event, &gAckState, INFYPowerModule, TurnlistCharge);

	//
	/////////////////////////////////////////////////////////////////////////////////////////??????????????????d
}


//
void INFYID_Set_DCout(uint32_t Volt, uint32_t Curr) //
{
	MonitorSet_BEG.ModuleVoltOut.all = Volt;

	if (MonitorSet_BEG.ModuleVoltOut.all != 0)
		{
		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress),
			 
			INFYModule_Cofg, 0x10, 0x1);
		msleep(20);
		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress),
			 
			INFYModule_Cofg, 0x11, 0x1);
		msleep(20);
		}

	MonitorSet_BEG.ModuleCurOut.all = Curr;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x10, 0x2);
	msleep(20);
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x11, 0x2);
	msleep(20);
}


void INFYID_Run_Word(unsigned char cmd) //A0:Ou流L籄1Lo2c蚾腶变L籄2LoA胪o腶变L?
{


	if ((cmd == 0xA0) || (cmd == 0xA1) || (cmd == 0xA2))
		{
		MonitorSet_BEG.Module_Word.all = cmd;
		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress),
			 
			INFYModule_Cofg, 0x21, 0x10);
		msleep(20);
		}



}


void INFYID_Start(void)
{
	MonitorSet_BEG.Module_Run.all = 0xA0;

	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x11, 0x10);
	msleep(20);

}


void INFYID_Stop(void)
{
	MonitorSet_BEG.Module_Run.all = 0xA1;

	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x11, 0x10);
	msleep(20);

}


//
void INFYID_INI(void)
{
	//??????
	INFYID_Set_DCout(0, 0);
	msleep(300);
	INFYID_Stop();
	msleep(300);

	//
	MonitorSet_BEG.Module_Num.all = 1;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x11, 0x3);
	msleep(300);

	//
	MonitorSet_BEG.Module_Walkin.all = 0xA1;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x11, 0x22);
	msleep(300);
	MonitorSet_BEG.Module_Add.all = 0xA1;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x11, 0x23);
	msleep(300);

	//
	MonitorSet_BEG.Module_AC_Volt.all = 220000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x1);
	msleep(300);

	//
	MonitorSet_BEG.Module_AC_HZ.all = 50000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x2);
	msleep(300);

	//
	MonitorSet_BEG.Module_AC_PF.all = 1000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x5);
	msleep(300);

	//
	MonitorSet_BEG.Module_3P.all = 0xA1;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x11);
	msleep(300);

	//
	MonitorSet_BEG.Module_Only.all = 0xA1;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x14);
	msleep(300);

	//
	MonitorSet_BEG.Module_LPSer.all = 0xA0;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x17);
	msleep(300);

	//
	MonitorSet_BEG.Module_AC_OV1.all = 240000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x20);
	msleep(300);

	//
	MonitorSet_BEG.Module_AC_OV1_T.all = 100000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x21);
	msleep(300);

	//
	MonitorSet_BEG.Module_AC_OV2.all = 250000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x22);
	msleep(300);

	//
	MonitorSet_BEG.Module_AC_OV2_T.all = 60000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x23);
	msleep(300);

	//
	MonitorSet_BEG.Module_AC_UV1.all = 200000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x24);
	msleep(300);

	//
	MonitorSet_BEG.Module_AC_UV1_T.all = 100000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x25);
	msleep(300);

	//
	MonitorSet_BEG.Module_AC_UV2.all = 180000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x26);
	msleep(300);

	//
	MonitorSet_BEG.Module_AC_UV2_T.all = 60000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x27);
	msleep(300);

	//
	MonitorSet_BEG.Module_HZ_OV1.all = 53000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x28);
	msleep(300);

	//
	MonitorSet_BEG.Module_HZ_OV1_T.all = 60000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x29);
	msleep(300);

	//
	MonitorSet_BEG.Module_HZ_OV2.all = 55000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x2A);
	msleep(300);

	//
	MonitorSet_BEG.Module_HZ_OV2_T.all = 60000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x2B);
	msleep(300);

	//
	MonitorSet_BEG.Module_HZ_UV1.all = 47000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x2C);
	msleep(300);

	//
	MonitorSet_BEG.Module_HZ_UV1_T.all = 60000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x2D);
	msleep(300);

	//
	MonitorSet_BEG.Module_HZ_UV2.all = 45000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x2E);
	msleep(300);

	//
	MonitorSet_BEG.Module_HZ_UV2_T.all = 60000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x2F);
	msleep(300);

	//
	MonitorSet_BEG.Module_AC_OV.all = 255000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x30);
	msleep(300);

	//
	MonitorSet_BEG.Module_AC_OV_T.all = 600000;
	SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x24, BroadcastAddress, MonitoringAdress), 
		INFYModule_Cofg, 0x21, 0x31);
	msleep(300);
	INFYID_Stop();
	msleep(300);

	//
	gSmartEnerg_sys.SmartEnergy_ini = 0;
	gSmartEnerg_sys.SmartEnergy_chager = 0;
	gSmartEnerg_sys.SmartEnergy_chager_ini = 0;
	gSmartEnerg_sys.SmartEnergy_chager_cell = 0;
	gSmartEnerg_sys.SmartEnergy_chager_pcs = 0;
	gSmartEnerg_sys.SmartEnergy_chager_end = 0;
	gSmartEnerg_sys.SmartEnergy_dischager = 0;
	gSmartEnerg_sys.SmartEnergy_dischag_ini = 0;
	gSmartEnerg_sys.SmartEnergy_dischag_cell = 0;
	gSmartEnerg_sys.SmartEnergy_dischag_pcs = 0;
	gSmartEnerg_sys.SmartEnergy_dischag_pcs_gird = 0;
	gSmartEnerg_sys.SmartEnergy_dischag_pcs_load = 0;
	gSmartEnerg_sys.SmartEnergy_dischag_end = 0;
	gSmartEnerg_sys.SmartEnergy_end = 0;
	gSmartEnerg_sys.SmartEnergy_err = 0;
	SmartEnerg_sta		= 0;
	gSmartEnerg_sys.SmartEnergy_end = 0;

	//
	//printf("SmartEnerg_io.OUT_3 1.\n");
	//SmartEnerg_io.OUT_3 = 0;
	SmartEnerg_io.OUT_4 = 0;

	//
}


//
unsigned char	DCModeNum_send = 0;
unsigned char	DCStopnum = 0;

//
int can_event1(void)
{
	unsigned char	i, k;

	while (can1_flg > 0) //if (can1_flg > 0)
		{
		can1_event			= can1_frame[can1_out];
		can1_data();

		if (can1_flg == 0)
			can1_flg = 0;
		else 
			can1_flg--;

		can1_out++;

		if (can1_out > FRAME)
			can1_out = 0;
		}

	//
	//
	/////////////////////////PowerModuleMonitorSet_BEG MonitorSet_BEG;
	////////////////void SetPowerModule_BEG(uint32_t ID, uint8_t Cmd, uint8_t BigCmd, uint8_t SmalCmd) //
	//for (k = 1; k <= DCModeNum; k++) //
		{
		/*
		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x23, DCModeNum_send, MonitoringAdress
			), 
			INFYModule_Read, 0x10, 0x1);
		msleep(100);
		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x23, DCModeNum_send, MonitoringAdress
			), 
			INFYModule_Read, 0x10, 0x2);
		msleep(100);
		*/
		for (i = 1; i <= 0x6; i++)
			{
			SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x23, DCModeNum_send, 
				MonitoringAdress), 
				INFYModule_Read, 0x11, i);

			msleep(20);
			}

		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x23, DCModeNum_send, 
			MonitoringAdress), 
			INFYModule_Read, 0x11, 0x10);
		msleep(20);
		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x23, DCModeNum_send, 
			MonitoringAdress), 
			INFYModule_Read, 0x11, 0x11);

		/*
		msleep(100);
		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x23, DCModeNum_send, MonitoringAdress
			), 
			INFYModule_Read, 0x11, 0x20);
		msleep(100);
		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x23, DCModeNum_send, MonitoringAdress
			), 
			INFYModule_Read, 0x11, 0x30);
		msleep(100);
		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x23, DCModeNum_send, MonitoringAdress
			), 
			INFYModule_Read, 0x11, 0x31);
		msleep(100);
		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x23, DCModeNum_send, MonitoringAdress
			), 
			INFYModule_Read, 0x11, 0x32);
		msleep(100);
		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x23, DCModeNum_send, MonitoringAdress
			), 
			INFYModule_Read, 0x11, 0x33);
		*/
		msleep(10);

		for (i = 1; i <= 0xf; i++)
			{
			//
			if (i != 0x8)
				{
				if (i != 0xC)
					{
					//
					SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x23, DCModeNum_send, MonitoringAdress),
						 
						INFYModule_Read, 0x21, i);
					}
				}

			msleep(20);
			}

		//

		/*
		msleep(100);
		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x23, DCModeNum_send, MonitoringAdress
			), 
			INFYModule_Read, 0x41, 0x01);
		msleep(100);
		SetPowerModule_BEG(INFYIDAssembleTool(Normal, SingleModuleProtocol, 0x23, DCModeNum_send, MonitoringAdress
			), 
			INFYModule_Read, 0x41, 0x02);
			*/
		}

	//
	DCModeNum_send++;

	if (DCModeNum_send >= (DCModeNum - 1))
		{

		DCModeNum_send		= 0;

		//
		gMonitorAck_BEG();

		//
		MonitorAck_BEG[DCModeNum - 1].Err.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Err_AC.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_AVolt.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_BVolt.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_CVolt.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_ABVolt.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_BCVolt.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_CAVolt.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_ACur.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_BCur.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_CCur.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_HZ.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_AllP.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_APower.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_BPower.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_CPower.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_AllLP.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_LAPower.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_LBPower.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Module_AC_LCPower.all = 0;
		MonitorAck_BEG[DCModeNum - 1].Temp = 0;

		//
		for (i = 0; i < (DCModeNum - 1); i++)
			{
			MonitorAck_BEG[DCModeNum - 1].Err.all = MonitorAck_BEG[DCModeNum - 1].Err.all | MonitorAck_BEG[i].Err.all;
			MonitorAck_BEG[DCModeNum - 1].Err_AC.all =
				 MonitorAck_BEG[DCModeNum - 1].Err_AC.all | MonitorAck_BEG[i].Err_AC.all;

			MonitorAck_BEG[DCModeNum - 1].Module_AC_AVolt.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_AVolt.all + (MonitorAck_BEG[i].Module_AC_AVolt.all / (DCModeNum - 1));
			MonitorAck_BEG[DCModeNum - 1].Module_AC_BVolt.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_BVolt.all + (MonitorAck_BEG[i].Module_AC_BVolt.all / (DCModeNum - 1));
			MonitorAck_BEG[DCModeNum - 1].Module_AC_CVolt.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_CVolt.all + (MonitorAck_BEG[i].Module_AC_CVolt.all / (DCModeNum - 1));

			MonitorAck_BEG[DCModeNum - 1].Module_AC_ABVolt.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_ABVolt.all + (MonitorAck_BEG[i].Module_AC_ABVolt.all / (DCModeNum - 1));
			MonitorAck_BEG[DCModeNum - 1].Module_AC_BCVolt.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_BCVolt.all + (MonitorAck_BEG[i].Module_AC_BCVolt.all / (DCModeNum - 1));
			MonitorAck_BEG[DCModeNum - 1].Module_AC_CAVolt.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_CAVolt.all + (MonitorAck_BEG[i].Module_AC_CAVolt.all / (DCModeNum - 1));

			MonitorAck_BEG[DCModeNum - 1].Module_AC_ACur.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_ACur.all + MonitorAck_BEG[i].Module_AC_ACur.all;
			MonitorAck_BEG[DCModeNum - 1].Module_AC_BCur.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_BCur.all + MonitorAck_BEG[i].Module_AC_BCur.all;
			MonitorAck_BEG[DCModeNum - 1].Module_AC_CCur.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_ACur.all + MonitorAck_BEG[i].Module_AC_CCur.all;

			MonitorAck_BEG[DCModeNum - 1].Module_AC_HZ.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_HZ.all + (MonitorAck_BEG[i].Module_AC_HZ.all / (DCModeNum - 1));

			//MonitorAck_BEG[DCModeNum - 1].Module_AC_AllP.all =
			//	 MonitorAck_BEG[DCModeNum - 1].Module_AC_AllP.all + (MonitorAck_BEG[i].Module_AC_AllP.all / (1));
			MonitorAck_BEG[DCModeNum - 1].Module_AC_APower.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_APower.all + (MonitorAck_BEG[i].Module_AC_APower.all / (1));
			MonitorAck_BEG[DCModeNum - 1].Module_AC_BPower.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_BPower.all + (MonitorAck_BEG[i].Module_AC_BPower.all / (1));
			MonitorAck_BEG[DCModeNum - 1].Module_AC_CPower.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_CPower.all + (MonitorAck_BEG[i].Module_AC_CPower.all / (1));
			MonitorAck_BEG[DCModeNum - 1].Module_AC_AllP.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_APower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_BPower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_CPower.all;

			//MonitorAck_BEG[DCModeNum - 1].Module_AC_AllLP.all =
			//	 MonitorAck_BEG[DCModeNum - 1].Module_AC_AllLP.all + (MonitorAck_BEG[i].Module_AC_AllLP.all / (1));
			MonitorAck_BEG[DCModeNum - 1].Module_AC_LAPower.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_LAPower.all + (MonitorAck_BEG[i].Module_AC_LAPower.all / (1));
			MonitorAck_BEG[DCModeNum - 1].Module_AC_LBPower.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_LBPower.all + (MonitorAck_BEG[i].Module_AC_LBPower.all / (1));
			MonitorAck_BEG[DCModeNum - 1].Module_AC_LCPower.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_LCPower.all + (MonitorAck_BEG[i].Module_AC_LCPower.all / (1));
			MonitorAck_BEG[DCModeNum - 1].Module_AC_AllLP.all =
				 MonitorAck_BEG[DCModeNum - 1].Module_AC_LAPower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_LBPower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_LCPower.all;

			MonitorAck_BEG[DCModeNum - 1].Temp =
				 MonitorAck_BEG[DCModeNum - 1].Temp + (MonitorAck_BEG[i].Temp / (DCModeNum - 1));


			}


		}

	//
	return 0;
}


//
int can_tast()
{
	//
	INFYID_INI();
	gPrintf();

	//
	while (1)
		{
		can_event0();
		usleep(100000);
		can_event1();
		usleep(100000);
		}

	return 0;
}


//
SetIncreaseModuleCurOut gSetIncreaseModuleCurOut;
SetIncreaseModuleVoltOut gSetIncreaseModuleVoltOut;

//
int can_send0()
{
	//
	DCModeNum			= PCS_DCModeNum;

	//
	CC2_DC_Run			= 0;						//B2408
	CCS_AC_DC			= 0;
	CDZ_charge_start	= 0;
	CF_Secc_DCAC_ChgMode = 0;
	CF_CP_Status		= 0;

	//
	while (1)
		{
		can_event0();								//BMS

		//
		Charging_Info();

		//chager
		//b2408 交直流座
		if (CC2_DC_Run == 1)
			{
			Bms_Awake(0xAA);
			Bms_Command(3);
			}
		else 
			{
			if (CC2_DC_Run == 2)
				{
				Bms_Awake(0x55);
				CC2_DC_Run			= 0;
				}
			}



		if (gSmartEnerg_sys.SmartEnergy_chager_ini)
			{
			if ((gBMSInfo.BMS_Sta & 0x03) == 0)
				{
				Bms_Awake(0xAA);
				}
			else 
				{
				Bms_Command(3);
				SmartEnerg_sta		= SmartEnergy_chager_cell;
				gSmartEnerg_sys.SmartEnergy_chager_ini = 0;
				}


			}

		if (gSmartEnerg_sys.SmartEnergy_chager_pcs)
			{
			Bms_Command(3);
			}

		if (gSmartEnerg_sys.SmartEnergy_chager_end)
			{
			Bms_Awake(0x55);

			if (((gBMSInfo.BMSCur.all <= 30050) && (gBMSInfo.BMSCur.all >= 29950)) || (gBMSInfo.BMSVolt.all <= 1000))
				{
				//gSmartEnerg_sys.SmartEnergy_chager_end = 0;
				SmartEnerg_sta		= SmartEnergy_end;

				//
				SmartEnerg_io.OUT_1 = 0;
				}
			}

		//
		//dischager
		if (gSmartEnerg_sys.SmartEnergy_dischag_ini)
			{
			if ((gBMSInfo.BMS_Sta & 0x03) == 0)
				{
				Bms_Awake(0xAA);
				}
			else 
				{
				Bms_Command(2);
				SmartEnerg_sta		= SmartEnergy_dischag_cell;
				gSmartEnerg_sys.SmartEnergy_dischag_ini = 0;
				}


			}

		if (gSmartEnerg_sys.SmartEnergy_dischag_cell)
			{
			Bms_Awake(0xAA);

			msleep(200);

			Bms_Command(2);

			}

		//printf("\ntcp_send.%d send_dat: %d \n", i,tcp_send.send_dat[i]);
		if (gSmartEnerg_sys.SmartEnergy_dischag_pcs)
			{
			Bms_Awake(0xAA);

			msleep(200);

			Bms_Command(2);
			}

		if (gSmartEnerg_sys.SmartEnergy_dischag_end)
			{
			Bms_Awake(0x55);

			if (((gBMSInfo.BMSCur.all <= 30050) && (gBMSInfo.BMSCur.all >= 29950)) || (gBMSInfo.BMSVolt.all <= 1000))
				{
				gSmartEnerg_sys.SmartEnergy_dischag_end = 0;
				SmartEnerg_sta		= SmartEnergy_end;

				//
				SmartEnerg_io.OUT_3 = 0;
				}
			}

		if ((gSmartEnerg_sys.SmartEnergy_end) || (gSmartEnerg_sys.SmartEnergy_err != 0) ||
			 (SmartEnerg_sta == SmartEnergy_ini))
			{
			if (CC2_Run == 0)
				{
				Bms_Awake(0x55);
				}
			}

		//
		usleep(100000);
		}

	printf("\ncan_send0 down.\n");
	pthread_exit(NULL);
	return 0;
}


//
uint32_t		PM2_Power = 0;

//
int can_send1()
{
	//
	uint32_t		cur_bark = 0;

	//
	DCModeNum			= PCS_DCModeNum;

	//
	INFYID_INI();

	//gPrintf();
	//
	while (1)
		{
		can_event1();

		//chager
		if (gSmartEnerg_sys.SmartEnergy_chager_cell)
			{
			//
			INFYID_INI();
			DCStopnum			= 0;

			//
			gSetIncreaseModuleCurOut.all =
				 (uint32_t) ((gBMSInfo.BMSCur_chager_max.all - 30000) * 100 / (DCModeNum - 1));
			gSetIncreaseModuleVoltOut.all = (uint32_t) (gBMSInfo.BMSVolt_chager_OV.all * 100);


			//
			INFYID_Set_DCout(gSetIncreaseModuleVoltOut.all, gSetIncreaseModuleCurOut.all);
			SmartEnerg_sta		= SmartEnergy_chager_pcs;
			gSmartEnerg_sys.SmartEnergy_chager_cell = 0;
			}

		if (gSmartEnerg_sys.SmartEnergy_chager_pcs)
			{
			INFYID_Run_Word(0xA0);

			//
			gSetIncreaseModuleCurOut.all =
				 (uint32_t) ((gBMSInfo.BMSCur_chager_max.all - 30000) * 100 / (DCModeNum - 1));
			gSetIncreaseModuleVoltOut.all = (uint32_t) (gBMSInfo.BMSVolt_chager_OV.all * 100);

			//???
			//(gAMT_Info[8].PMActivePower.all * 10 / (gBMSInfo.BMSVolt.all))
			if (gBMSInfo.BMSVolt.all != 0)
				{
				cur_bark			= (200000000 / gBMSInfo.BMSVolt.all); //B2401

				//
				cur_bark			=
					 cur_bark - (gAMT_Info[8].PMActivePower.all * 10000 / (gBMSInfo.BMSVolt.all) / (DCModeNum - 1)); //B2401 充电功率自适应-交流充放电同时存在
				}
			else 
				{
				cur_bark			= 40000;
				}

			//
			if (gSetIncreaseModuleCurOut.all > cur_bark)
				{
				gSetIncreaseModuleCurOut.all = cur_bark;
				}

			//配置功率计算
			if (gBMSInfo.BMSVolt.all != 0)
				{
				cur_bark			= (SYS_SET_Charg_Power * 10000000) / gBMSInfo.BMSVolt.all / (DCModeNum - 1);

				if (gSetIncreaseModuleCurOut.all > cur_bark)
					{
					gSetIncreaseModuleCurOut.all = cur_bark;
					}
				}

			printf("\ngSetIncreaseModuleOut: %d \n", gSetIncreaseModuleCurOut.all / 1000 * gBMSInfo.BMSVolt.all / 10);

			//
			//
			INFYID_Set_DCout(gSetIncreaseModuleVoltOut.all, gSetIncreaseModuleCurOut.all);

			//SmartEnerg_sta		= SmartEnergy_chager_end;
			INFYID_Start();
			}

		if (gSmartEnerg_sys.SmartEnergy_chager_end)
			{
			INFYID_Stop();
			gSmartEnerg_sys.SmartEnergy_chager_pcs = 0;

			if ((MonitorAck_BEG[(DCModeNum - 1)].Module_AC_CCur.all + MonitorAck_BEG[(DCModeNum - 1)].Module_AC_BCur.all + MonitorAck_BEG[(DCModeNum - 1)].Module_AC_ACur.all) <=
				 210) //B2401 每一相7A，总共21A
				{
				SmartEnerg_io.OUT_1 = 0;

				//SmartEnerg_io.OUT_3 = 0;
				//
				gSmartEnerg_sys.SmartEnergy_chager_end = 0;
				SmartEnerg_sta		= SmartEnergy_end;
				}
			}

		//dischager
		if (gSmartEnerg_sys.SmartEnergy_dischag_cell)
			{
			//
			INFYID_INI();
			DCStopnum			= 0;

			//
			//
			gSetIncreaseModuleCurOut.all =
				 (uint32_t) ((gBMSInfo.BMSCur_dischge_max.all - 30000) * 100 / (DCModeNum - 1));

			//gSetIncreaseModuleVoltOut.all = gBMSInfo.BMSVolt_dischge_UV.all * 100;
			//
			INFYID_Set_DCout(gSetIncreaseModuleVoltOut.all, gSetIncreaseModuleCurOut.all);
			SmartEnerg_sta		= SmartEnergy_dischag_pcs;
			gSmartEnerg_sys.SmartEnergy_dischag_cell = 0;
			}

		if (gSmartEnerg_sys.SmartEnergy_dischag_pcs) //
			{
			INFYID_Run_Word(0xA2);

			//
			gSetIncreaseModuleCurOut.all =
				 (uint32_t) ((gBMSInfo.BMSCur_dischge_max.all - 30000) * 100 / (DCModeNum - 1));

			cur_bark			= gBMSInfo.BMSVolt.all * (gSetIncreaseModuleCurOut.all / 100);

			//printf("\ngcur_bark: %d \n", cur_bark);
			if (cur_bark > 700000)
				gSetIncreaseModuleCurOut.all = 75000; //BEG1K075

			//gSetIncreaseModuleVoltOut.all = gBMSInfo.BMSVolt_dischge_UV.all * 100;
			//gSetIncreaseModuleVoltOut.all = 780000;
			//if(gBMSInfo.BMSVolt.all<=5400) gBMSInfo.BMSVolt.all = 5400;
			//gSetIncreaseModuleVoltOut.all = gBMSInfo.BMSVolt.all * 100;
			//
			gSetIncreaseModuleVoltOut.all = 0;

			//if (((gAMT_Info[5].PMAPhaseVolt.all >= 1500) && (gAMT_Info[5].PMBPhaseVolt.all >= 1500) &&
			//	 (gAMT_Info[5].PMCPhaseVolt.all >= 1500)) ||
			//	 ((MonitorAck_BEG[1].Module_AC_AVolt.all >= 1500) &&
			//	 (MonitorAck_BEG[1].Module_AC_BVolt.all >= 1500) && (MonitorAck_BEG[1].Module_AC_CVolt.all >= 1500)))
			if ((MonitorAck_BEG[(DCModeNum - 1)].Module_AC_AVolt.all >= 1500) &&
				 (MonitorAck_BEG[(DCModeNum - 1)].Module_AC_BVolt.all >= 1500) &&
				 (MonitorAck_BEG[(DCModeNum - 1)].Module_AC_CVolt.all >= 1500))
				{
				//
				SmartEnerg_sta		= SmartEnergy_dischag_pcs_load;
				gSmartEnerg_sys.SmartEnergy_dischag_pcs = 0;
				}

			//
			INFYID_Set_DCout(gSetIncreaseModuleVoltOut.all, gSetIncreaseModuleCurOut.all);

			//SmartEnerg_sta		= SmartEnergy_chager_end;

			/*
			if ((gAMT_Info[3].PMAPhaseCur.all > 180) || (gAMT_Info[2].PMAPhaseCur.all > 180) ||
				 (gAMT_Info[2].PMBPhaseCur.all > 180) || (gAMT_Info[2].PMCPhaseCur.all > 180) ||
				 (gAMT_Info[1].PMAPhaseCur.all > 350) || (gAMT_Info[1].PMBPhaseCur.all > 350) ||
				 (gAMT_Info[1].PMCPhaseCur.all > 350))
				{
				SmartEnerg_sta		= SmartEnergy_dischag_end;
				INFYID_Stop();
				SmartEnerg_io.OUT_4 = 1;
				}
			*/
			/*
			printf("\ngMonitorAck_BEG[0].Module_AC_AVolt: %d \n", MonitorAck_BEG[0].Module_AC_AVolt.all);
			printf("\ngMonitorAck_BEG[0].Module_AC_BVolt: %d \n", MonitorAck_BEG[0].Module_AC_BVolt.all);
			printf("\ngMonitorAck_BEG[0].Module_AC_CVolt: %d \n", MonitorAck_BEG[0].Module_AC_CVolt.all);
			printf("\ngMonitorAck_BEG[1].Module_AC_AVolt: %d \n", MonitorAck_BEG[1].Module_AC_AVolt.all);
			printf("\ngMonitorAck_BEG[1].Module_AC_BVolt: %d \n", MonitorAck_BEG[1].Module_AC_BVolt.all);
			printf("\ngMonitorAck_BEG[1].Module_AC_CVolt: %d \n", MonitorAck_BEG[1].Module_AC_CVolt.all);
			printf("\ngAMT_Info[3].PMAPhaseCur.all: %d \n", gAMT_Info[3].PMAPhaseCur.all);
			printf("\ngAMT_Info[2].PMAPhaseCur.all: %d \n", gAMT_Info[2].PMAPhaseCur.all);
			printf("\ngAMT_Info[2].PMBPhaseCur.all: %d \n", gAMT_Info[2].PMBPhaseCur.all);
			printf("\ngAMT_Info[2].PMCPhaseCur.all: %d \n", gAMT_Info[2].PMCPhaseCur.all);
			printf("\ngAMT_Info[1].PMAPhaseCur.all: %d \n", gAMT_Info[1].PMAPhaseCur.all);
			printf("\ngAMT_Info[1].PMBPhaseCur.all: %d \n", gAMT_Info[1].PMBPhaseCur.all);
			printf("\ngAMT_Info[1].PMCPhaseCur.all: %d \n", gAMT_Info[1].PMCPhaseCur.all);
			printf("\ngDCDCACur.all: %d \n", 
				(MonitorAck_BEG[0].Module_AC_ACur.all + MonitorAck_BEG[1].Module_AC_ACur.all));
			printf("\ngDCDCBCur.all: %d \n", 
				(MonitorAck_BEG[0].Module_AC_BCur.all + MonitorAck_BEG[1].Module_AC_BCur.all));
			printf("\ngDCDCCCur.all: %d \n", 
				(MonitorAck_BEG[0].Module_AC_CCur.all + MonitorAck_BEG[1].Module_AC_CCur.all));
			*/
			INFYID_Start();
			}

		if (gSmartEnerg_sys.SmartEnergy_dischag_pcs_load) //????
			{
			INFYID_Run_Word(0xA2);

			//
			gSetIncreaseModuleCurOut.all =
				 (uint32_t) ((gBMSInfo.BMSCur_dischge_max.all - 30000) * 100 / (DCModeNum - 1));
			cur_bark			= gBMSInfo.BMSVolt.all * (gSetIncreaseModuleCurOut.all / 100);

			//printf("\ngcur_bark: %d \n", cur_bark);
			if (cur_bark > 700000)
				gSetIncreaseModuleCurOut.all = 75000; //BEG1K075

			//gSetIncreaseModuleVoltOut.all = gBMSInfo.BMSVolt_dischge_UV.all * 100;
			//gSetIncreaseModuleVoltOut.all = 780000;
			//if(gBMSInfo.BMSVolt.all<=5400) gBMSInfo.BMSVolt.all = 5400;
			//gSetIncreaseModuleVoltOut.all = gBMSInfo.BMSVolt.all * 100;
			//
			gSetIncreaseModuleVoltOut.all = 0;

			//
			INFYID_Set_DCout(gSetIncreaseModuleVoltOut.all, gSetIncreaseModuleCurOut.all);

			//SmartEnerg_sta		= SmartEnergy_chager_end;

			/*
			if ((gAMT_Info[3].PMAPhaseCur.all > 180) || (gAMT_Info[2].PMAPhaseCur.all > 180) ||
				 (gAMT_Info[2].PMBPhaseCur.all > 180) || (gAMT_Info[2].PMCPhaseCur.all > 180) ||
				 (gAMT_Info[1].PMAPhaseCur.all > 350) || (gAMT_Info[1].PMBPhaseCur.all > 350) ||
				 (gAMT_Info[1].PMCPhaseCur.all > 350))
				 
			if ((gAMT_Info[3].PMAPhaseCur.all > 180) || (gAMT_Info[1].PMAPhaseCur.all > 350) ||
				 (gAMT_Info[1].PMBPhaseCur.all > 350) || (gAMT_Info[1].PMCPhaseCur.all > 350))
				{
				SmartEnerg_sta		= SmartEnergy_dischag_end;
				INFYID_Stop();
				SmartEnerg_io.OUT_4 = 1;
				}
			*/
			if ((MonitorAck_BEG[(DCModeNum - 1)].Module_AC_AVolt.all < 1100) ||
				 (MonitorAck_BEG[(DCModeNum - 1)].Module_AC_BVolt.all < 1100) ||
				 (MonitorAck_BEG[(DCModeNum - 1)].Module_AC_CVolt.all < 1100))
				{
				//
				DCStopnum++;
				SmartEnerg_sta		= SmartEnergy_dischag_pcs;
				gSmartEnerg_sys.SmartEnergy_dischag_pcs_load = 0;
				}

			//
			if (DCStopnum > 3)
				{
				SmartEnerg_sta		= SmartEnergy_dischag_end;
				INFYID_Stop();
				SmartEnerg_io.OUT_4 = 1;
				gSmartEnerg_sys.SmartEnergy_dischag_pcs_load = 0;
				}
			else 
				{
				INFYID_Start();
				}

			/*
						printf("\ngMonitorAck_BEG[0].Module_AC_AVolt: %d \n", MonitorAck_BEG[0].Module_AC_AVolt.
				all);
						printf("\ngMonitorAck_BEG[0].Module_AC_BVolt: %d \n", MonitorAck_BEG[0].Module_AC_BVolt.
				all);
						printf("\ngMonitorAck_BEG[0].Module_AC_CVolt: %d \n", MonitorAck_BEG[0].Module_AC_CVolt.
				all);
						printf("\ngMonitorAck_BEG[1].Module_AC_AVolt: %d \n", MonitorAck_BEG[1].Module_AC_AVolt.
				all);
						printf("\ngMonitorAck_BEG[1].Module_AC_BVolt: %d \n", MonitorAck_BEG[1].Module_AC_BVolt.
				all);
						printf("\ngMonitorAck_BEG[1].Module_AC_CVolt: %d \n", MonitorAck_BEG[1].Module_AC_CVolt.
				all);
						printf("\ngAMT_Info[3].PMAPhaseCur.all: %d \n", gAMT_Info[3].PMAPhaseCur.all);
						printf("\ngAMT_Info[2].PMAPhaseCur.all: %d \n", gAMT_Info[2].PMAPhaseCur.all);
						printf("\ngAMT_Info[2].PMBPhaseCur.all: %d \n", gAMT_Info[2].PMBPhaseCur.all);
						printf("\ngAMT_Info[2].PMCPhaseCur.all: %d \n", gAMT_Info[2].PMCPhaseCur.all);
						printf("\ngAMT_Info[1].PMAPhaseCur.all: %d \n", gAMT_Info[1].PMAPhaseCur.all);
						printf("\ngAMT_Info[1].PMBPhaseCur.all: %d \n", gAMT_Info[1].PMBPhaseCur.all);
						printf("\ngAMT_Info[1].PMCPhaseCur.all: %d \n", gAMT_Info[1].PMCPhaseCur.all);
						printf("\ngDCDCACur.all: %d \n", 
							(MonitorAck_BEG[0].Module_AC_ACur.all + MonitorAck_BEG[1].Module_AC_ACur.all));
						printf("\ngDCDCBCur.all: %d \n", 
							(MonitorAck_BEG[0].Module_AC_BCur.all + MonitorAck_BEG[1].Module_AC_BCur.all));
						printf("\ngDCDCCCur.all: %d \n", 
							(MonitorAck_BEG[0].Module_AC_CCur.all + MonitorAck_BEG[1].Module_AC_CCur.all));
							*/
			//
			SmartEnerg_io.OUT_3 = 1;				//B2401

			//
			}

		if (gSmartEnerg_sys.SmartEnergy_dischag_end)
			{
			INFYID_Stop();
			gSmartEnerg_sys.SmartEnergy_dischag_pcs_load = 0;
			gSmartEnerg_sys.SmartEnergy_dischag_pcs_gird = 0;
			}

		if ((gSmartEnerg_sys.SmartEnergy_err != 0) || (gBMSInfo.Protection.all != 0))
			{
			//SmartEnerg_sta		= SmartEnergy_end;
			//INFYID_Stop();
			SmartEnerg_io.OUT_4 = 1;

			if (((SmartEnerg_sta == SmartEnergy_chager) || (SmartEnerg_sta == SmartEnergy_chager_ini) ||
				 (SmartEnerg_sta == SmartEnergy_chager_cell) || (SmartEnerg_sta == SmartEnergy_chager_pcs)) &&
				 (gBMSInfo.BMSCur_chager_max.all != 0))
				{
				//
				}
			else 
				{
				SmartEnerg_sta		= SmartEnergy_end;
				INFYID_Stop();
				}
			}

		/*
		 if (((gBMSInfo.Alarm.all & 0x1) == 0x1) || ((gBMSInfo.Alarm.all & 0x4) == 0x4) ||
			  ((gBMSInfo.Alarm.all & 0x200) == 0x200) || ((gBMSInfo.Alarm.all & 0x400) == 0x400))
			 {
			 SmartEnerg_sta		= SmartEnergy_end;
			 INFYID_Stop();
			 SmartEnerg_io.OUT_4 = 1;
			 }
		 */
		if ((gSmartEnerg_sys.SmartEnergy_end) || (SmartEnerg_sta == SmartEnergy_ini))
			{
			INFYID_Stop();
			}

		//
		BCU_COM_num++;

		if (BCU_COM_num >= 200)
			{
			SmartEnerg_sta		= SmartEnergy_end;
			INFYID_Stop();
			SmartEnerg_io.OUT_4 = 1;

			//
			BCU_COM_num 		= 200;
			}

		//
		//
		usleep(100000);
		}

	printf("\ncan_send1 down.\n");
	pthread_exit(NULL);
	return 0;
}


//
//
