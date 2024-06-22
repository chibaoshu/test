
#include <termios.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

#include <signal.h>

//
#include "cmd_queue_HMI.h"
#include "hmi_driver.h"
#include "cmd_process.h"
#include "stdio.h"
#include "string.h"
//
#include "DBsystem.h"
//
#include "SmartEnergy.h"
#include "canecho.h"
#include "sqlite.h"

///////////////////////////////µç±íÐÅÏ¢////////////////////////////////////////
#include "AMT_Moni.h"
///////////////////////////////////////////////////////////////////////
//
#define msleep(n)				usleep(n*1000)

#define delay_ms(delay) 		msleep(delay)

//char test[1024]="uart test.....bz";
volatile int	fd;
char *			dev = NULL;

pthread_mutex_t mut;
fd_set			rd;
int 			nread, retval;
unsigned char	msg[2048];


struct timeval timeout =
{
	0, 100
};


volatile pthread_t thread[2];
volatile const int READ_THREAD_ID = 0;
volatile const int SEND_THREAD_ID = 1;
volatile int	COM_READ_STATU = 1;
volatile int	COM_SEND_STATU = 1;

volatile int	sendnum = 0, sendnum_times = 0, recenum = 0, recenum_times = 0, cornum = 0, cornum_times = 0;

//----------------------------------------------------------------------------------
#define start					2
#define stop					1
#define Stop					0
volatile uint32 timer_tick_count_[100] =
{
	0
};


//¶¨Ê±Æ÷½ÚÅÄ
volatile uint8	buykeyflag = 0;
volatile uint8	buttonFlag;
uint8			lightG = 10;
void UpdateUI(void);
void UpdateSerRec0(void);
void UpdateSerRec1(void);
void UpdateDateRec0(void);
void UpdateDateRec1(void);
void UpdateProductRec0(void);
void UpdateProductRec1(void);

void ProcessMessage(PCTRL_MSG msg, uint16 size);

//----------------------------------------------------------------------------------
static speed_t getBaudrate(int baudrate)
{
	switch (baudrate)
		{
		case 0:
			return B0;

		case 50:
			return B50;

		case 75:
			return B75;

		case 110:
			return B110;

		case 134:
			return B134;

		case 150:
			return B150;

		case 200:
			return B200;

		case 300:
			return B300;

		case 600:
			return B600;

		case 1200:
			return B1200;

		case 1800:
			return B1800;

		case 2400:
			return B2400;

		case 4800:
			return B4800;

		case 9600:
			return B9600;

		case 19200:
			return B19200;

		case 38400:
			return B38400;

		case 57600:
			return B57600;

		case 115200:
			return B115200;

		case 230400:
			return B230400;

		case 460800:
			return B460800;

		case 500000:
			return B500000;

		case 576000:
			return B576000;

		case 921600:
			return B921600;

		case 1000000:
			return B1000000;

		case 1152000:
			return B1152000;

		case 1500000:
			return B1500000;

		case 2000000:
			return B2000000;

		case 2500000:
			return B2500000;

		case 3000000:
			return B3000000;

		case 3500000:
			return B3500000;

		case 4000000:
			return B4000000;

		default:
			return - 1;
		}
}



int OpenDev(char * Dev)
{
	speed_t 		speed;

	int 			i	= 0;
	int 			fdt, c = 0, num;

	struct termios oldtio, newtio;
	speed				= getBaudrate(38400);
	fdt 				= open(Dev, O_RDWR | O_NONBLOCK | O_NOCTTY | O_NDELAY);

	if (fdt < 0)
		{
		perror(Dev);
		exit(1);
		}

	//save to oldtio
	tcgetattr(fdt, &oldtio);

	//clear newtio
	bzero(&newtio, sizeof(newtio));

	//newtio.c_cflag = speed|CS8|CLOCAL|CREAD|CRTSCTS;
	newtio.c_cflag		= speed | CS8 | CLOCAL | CREAD;

	newtio.c_iflag		= IGNPAR;

	//
	//newtio.c_iflag&= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON | IICANON |ISIG );
	newtio.c_iflag		&= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

	//
	newtio.c_lflag		&= ~(ICANON | ECHO | ECHOE | ISIG | IGNCR);

	//
	newtio.c_oflag		= 0;

	//printf("newtio.c_cflag=%x\n",newtio.c_cflag);
	tcflush(fdt, TCIFLUSH);
	tcsetattr(fdt, TCSANOW, &newtio);
	tcgetattr(fdt, &oldtio);

	//printf("oldtio.c_cflag=%x\n",oldtio.c_cflag);
	return fdt;
}


void read_port(void)
{
	//
	int 			msg_i = 0;
	uint8			read_hmi;

	//
	FD_ZERO(&rd);
	FD_SET(fd, &rd);

	// timeout.tv_sec = 1;
	// timeout.tv_usec = 0;
	retval				= select(fd + 1, &rd, NULL, NULL, &timeout);

	switch (retval)
		{
		case 0:
			// printf("no data input within  1s.\n");
			break;

		case - 1:
			perror("select");
			break;

		default:
			if ((nread = read(fd, msg, 280)) > 0) //if((nread=read(fd,read_hmi,1))>0)
				{
				//
				//printf("msg: %d \n", nread);
				//
				//queue_push(msg[0]);
				msg_i				= 0;			//

				while (nread > 0)
					{
					queue_push(msg[msg_i]);

					//
					//printf("%02X", msg[msg_i]);
					//
					msg_i++;
					nread--;
					}

				//
				if (recenum < 65535)
					{
					recenum++;
					}
				else 
					{
					recenum_times++;
					recenum 			= 1;
					}
				}

			break;
		}
}


void * com_read(void * pstatu)
{
	int 			o;

	while (COM_READ_STATU)
		{
		pthread_mutex_lock(&mut);
		read_port();
		pthread_mutex_unlock(&mut);
		}

	printf("\ncom_read down.\n");
	pthread_exit(NULL);
}


//
#define TIME_100MS				10																  //100ÂºÃÃƒÃ«(10Â¸Ã¶ÂµÂ¥ÃŽÂ»)

volatile uint32 timer_tick_count = 0; //Â¶Â¨ÃŠÂ±Ã†Ã·Â½ÃšÃ…Ã„

uint8			cmd_buffer[CMD_MAX_SIZE]; //Ã–Â¸ÃÃ®Â»ÂºÂ´Ã¦
static uint16	current_screen_id = 0; //ÂµÂ±Ã‡Â°Â»Â­ÃƒÃ¦ID
static int32	progress_value = 0; //Â½Ã¸Â¶ÃˆÃŒÃµÂ²Ã¢ÃŠÃ”Ã–Âµ
static int32	test_value = 0; //Â²Ã¢ÃŠÃ”Ã–Âµ
static uint8	update_en = 0; //Â¸Ã¼ÃÃ‚Â±ÃªÂ¼Ã‡
static int32	meter_flag = 0; //Ã’Ã‡Â±Ã­Ã–Â¸Ã•Ã«ÃÃ¹Â·ÂµÂ±ÃªÃ–Â¾ÃŽÂ»
static int32	num = 0; //Ã‡ÃºÃÃŸÂ²Ã‰Ã‘Ã¹ÂµÃ£Â¼Ã†ÃŠÃ½
static int		sec = 1; //ÃŠÂ±Â¼Ã¤ÃƒÃ«
static int32	curves_type = 0; //Ã‡ÃºÃÃŸÂ±ÃªÃ–Â¾ÃŽÂ»  0ÃŽÂªÃ•Ã½ÃÃ’Â²Â¨Â£Â¬1ÃŽÂªÂ¾Ã¢Â³ÃÂ²Â¨
static int32	second_flag = 0; //ÃŠÂ±Â¼Ã¤Â±ÃªÃ–Â¾ÃŽÂ»
static int32	icon_flag = 0; //ÃÂ¼Â±ÃªÂ±ÃªÃ–Â¾ÃŽÂ»
static uint8	Select_H; //Â»Â¬Â¶Â¯Ã‘Â¡Ã”Ã±ÃÂ¡ÃŠÂ±
static uint8	Select_M; //Â»Â¬Â¶Â¯Ã‘Â¡Ã”Ã±Â·Ã–Ã–Ã“
static uint8	Last_H; //Ã‰ÃÃ’Â»Â¸Ã¶Ã‘Â¡Ã”Ã±ÃÂ¡ÃŠÂ±
static uint8	Last_M; //Ã‰ÃÃ’Â»Â¸Ã¶Ã‘Â¡Ã”Ã±Â·Ã–Ã–Ã“
static int32	Progress_Value = 0; //Â½Ã¸Â¶ÃˆÃŒÃµÂµÃ„Ã–Âµ

void UpdateUI(void);

//
void * com_send(void * p)
{
	int 			i;

	//
	static int32	test_value = 0; 				//??????
	uint32			timer_tick_last_update = 0; 	//???????$)A(9?????!@??
	qsize			size = 0;

	//
	//??????????????????
	queue_reset();

	//
	//printf("send run!\n");
	while (COM_SEND_STATU)
		{
		//write(fd, test, strlen(test));
		//
		//if(sendnum<65535)
		//	{
		//		sendnum++;
		//	}
		//	else
		//	{
		//		sendnum_times++;
		//		sendnum=1;
		//	}
		//
		size				= queue_find_cmd(cmd_buffer, CMD_MAX_SIZE);

		//??????????????????????
		//

		/*
		printf("\ncmd_buffer_size: %d \n", size);

		//
		printf("\ncmd_buffer:\n");

		//
		i					= 0;

		while (i < size)
			{
			printf("%02X", cmd_buffer[i]);
			i					= i + 1;
			}

		//
		*/
		//
		//if((size>0)&&(cmd_buffer[1]!=0x07))
		if (size > 0) //?????????? ???$)A!c?????????????(2?("??
			{
			ProcessMessage((PCTRL_MSG) cmd_buffer, size);

			//printf("send 2!\n");//???????$)A(*
			}
		else if ((size > 0) && (cmd_buffer[1] == 0x07)) //??????????0x07???$)A(*????STM32
			{
			// __disable_fault_irq();
			//VIC_SystemReset();
			}

		//
		msleep(1);

		}

	printf("\ncom_send down.\n");
	pthread_exit(NULL);
}


//
void SendChar(uchar t)
{
	if (COM_SEND_STATU)
		{
		write(fd, &t, 1);

		//printf("send_data: %c \n",t);
		//
		}
}


//
int start_thread_func(void * (*func) (void *), pthread_t * pthread, void * par, int * COM_STATU)
{
	*COM_STATU			= 1;
	memset(pthread, 0, sizeof(pthread_t));
	int 			temp;

	/*creat thread*/
	if ((temp = pthread_create(pthread, NULL, func, par)) != 0)
		{
		printf("creat thread failer!\n");
		}
	else 
		{
		int 			id	= pthread_self();

		printf("%s,creat thread %lu sucess\n", dev, *pthread);
		}

	return temp;
}


void SignHandler(int iSignNo)
{
	/*
		int 			tmp_t = 0;

		COM_SEND_STATU		= 0;
		msleep(1000);
		COM_READ_STATU		= 0;

		msleep(1000);

		//printf("%s,stop send,sendnum=%d,receivenum=%d\n",dev,sendnum*32,recenum);
		while (tmp_t < 50000)
			{
			read_port();
			tmp_t++;
			}

		printf("\n%s,Send: %d ,Receive: %d \n", dev, (sendnum_times * 65535 + sendnum) * 32, 
			(recenum_times * 65535 + recenum));
		exit(1);
		*/
}


//HMI
///////////////////////////////////////////////////////////////////////Charging RUN
uint8			CDZ_charge_start = 0;
uint8			CDZ_charge_start_isOk = 0;

#define CDZ_charge_start_isON	!(CDZ_charge_start_isOk)

uint8			CDZ_charge_stop = 1;
uint8			CDZ_harge_stop_isOk = 0;

///////////////////////////////////////////////////////////////////////ESS RUN
uint8			AC_Discharge_start = 0;
uint8			AC_Discharge_start_isOk = 0;

#define AC_Discharge_start_isON !(AC_Discharge_start_isOk)

uint8			AC_Discharge_stop = 0;
uint8			AC_Discharge_stop_isOk = 0;

//#define AC_Discharge_stop_isON	!(AC_Discharge_start_isOk)
///////////////////////////////////////////////////////////////////////Grid RUN
uint8			AC_Charge_start = 0;
uint8			AC_Charge_start_isOk = 0;

//#define AC_Discharge_start_isON !(AC_Discharge_start_isOk)
uint8			AC_Charge_stop = 0;
uint8			AC_Charge_stop_isOk = 0;

//#define AC_Charge_stop_isON	!(AC_Discharge_start_isOk)
///
///////////////////////////////////////////////////////////////////////
uint8			DEG_charge_start = 0;
uint8			DEG_charge_start_isOk = 0;

//#define DEG_charge_start_isON !(DEG_charge_start_isOk)
uint8			DEG_charge_stop = 0;
uint8			DEG_charge_stop_isOk = 0;

//#define DEG_charge_stop_isON	!(DEG_charge_start_isOk)
uint8			WL_Charge_start = 0;
uint8			WL_Charge_start_isOk = 0;

//#define WL_Discharge_start_isON !(DEG_charge_start_isOk)
uint8			WL_Charge_stop = 0;
uint8			WL_Charge_stop_isOk = 0;

//#define WL_Charge_stop_isON	!(DEG_charge_start_isOk)
//
uint8			Page63 = 0;
uint8			Page64 = 0;
uint8			Page65 = 0;
uint8			Page74 = 0;
uint8			Page72 = 0;
uint8			Page73 = 0;
uint8			Page70 = 0;
uint8			Page71 = 0;
uint8			Page75 = 0;
uint8			Page78 = 0;
uint8			Page113 = 0;
uint8			Page114 = 0;



//
uint16			Enquiries_Page82 = 0;
uint16			Enquiries_Page84 = 0;
uint16			Enquiries_Page85 = 0;
uint8			Page82 = 0;
uint8			Page84 = 0;
uint8			Page85 = 0;


uint8			SYS_SET_Page81 = 0;
uint8			SYS_SET_time_flg = 0;
uint8			SYS_info_flg = 0;

uint8			SYS_faul_flg = 0;


uint8			SYS_SET_time[19] =
{
	0
};


uint8			SYS_SET_Page95 = 0;
uint8			SYS_SET_Page96 = 0;
uint8			SYS_SET_Page97 = 0;
uint8			SYS_SET_Page98 = 0;
uint8			SYS_SET_Page99 = 0;
uint8			Page81 = 0;
uint8			Page95 = 0;
uint8			Page96 = 0;
uint8			Page97 = 0;
uint8			Page98 = 0;
uint8			Page99 = 0;


//
uint8			Page70_back = 0;
uint8			Page71_back = 0;
uint8			Page72_back = 0;
uint8			Page75_back = 0;

uint8			Page73_back = 0;
uint8			Page73_last = 0;
uint8			Page73_next = 0;

uint8			Page74_back = 0;
uint8			Page74_last = 0;
uint8			Page74_next = 0;

///////////////////////////////////////////////²ÎÊýÉèÖÃ±äÁ¿
////SET
uint64_t		Set_date = 0;

uint64_t		Set_time = 0;

////SAVE
uint64_t		Save_date = 0;

uint64_t		Save_time = 0;

///////////////////////////////////////////////////////////B2408
uint8			Page204_B2408 = 0;

///////////////////////////////////////////////²ÎÊýÉèÖÃ±äÁ¿¡ª¡ªend
///////////////////////////////////////////////Êý¾ÝÏÔÊ¾±äÁ¿
///////////////////////////////////////////////Êý¾ÝÏÔÊ¾±äÁ¿¡ª¡ªend
///
void Page_ini(void)
{
	Page63				= 0;
	Page64				= 0;
	Page74				= 0;
	Page72				= 0;
	Page73				= 0;
	Page70				= 0;
	Page71				= 0;
	Page75				= 0;
	Page65				= 0;
	Page78				= 0;
	Page81				= 0;
	Page82				= 0;
	Page113 			= 0;
	Page114 			= 0;

	//
	Page204_B2408		= 0;

}


////
void Page_bln(void)
{

	Page70_back 		= 0;
	Page71_back 		= 0;
	Page72_back 		= 0;
	Page75_back 		= 0;

	Page73_back 		= 0;
	Page73_last 		= 0;
	Page73_next 		= 0;

	Page74_back 		= 0;
	Page74_last 		= 0;
	Page74_next 		= 0;

}


void Page_set_info(void)
{
	Enquiries_Page82	= 0;
	Enquiries_Page84	= 0;
	Enquiries_Page85	= 0;
	Page81				= 0;
	Page95				= 0;
	Page96				= 0;
	Page97				= 0;
	Page98				= 0;
	Page99				= 0;

	SYS_SET_Page81		= 0;
	SYS_SET_Page95		= 0;
	SYS_SET_Page96		= 0;
	SYS_SET_Page97		= 0;
	SYS_SET_Page98		= 0;
	SYS_SET_Page99		= 0;
	Page82				= 0;
	Page84				= 0;
	Page85				= 0;

}


void Page_set_page(void)
{

	Page81				= 0;
	Page95				= 0;
	Page96				= 0;
	Page97				= 0;
	Page98				= 0;
	Page99				= 0;
	Page82				= 0;
	Page84				= 0;
	Page85				= 0;

}


void HMI_sys_ini(void)
{
	//MAX12×Ö·û
	ShowRecSerStrings_ERR(7, 0x82, 0x1160, "HW ver:");
	usleep(20000);
	ShowRecSerStrings_ERR(10, 0x82, 0x1170, "HW-V240120");
	usleep(20000);
	ShowRecSerStrings_ERR(7, 0x82, 0x1180, "SW ver:");
	usleep(20000);
	ShowRecSerStrings_ERR(12, 0x82, 0x1190, "B2408.2V0614");
	usleep(20000);

	//
	ShowRecSerStrings_ERR(7, 0x82, 0x11A0, "EMS_IP:");
	usleep(20000);
	ShowRecSerStrings_ERR(10, 0x82, 0x11B0, "C0.A8.1.E8");
	usleep(20000);
	ShowRecSerStrings_ERR(8, 0x82, 0x11C0, "EMS_MAK:");
	usleep(20000);
	ShowRecSerStrings_ERR(10, 0x82, 0x11D0, "FF.FF.FF.0");
	usleep(20000);

	//
	ShowRecSerStrings_ERR(11, 0x82, 0x11E0, "MODE_Num*P:");
	usleep(20000);

	ShowRecSerValue(0x82, 0x2580, SYS_SET_PCS_Num);
	usleep(20000);
	ShowRecSerStrings_ERR(5, 0x82, 0x11F0, "3*22KW");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1200, "ESS:");
	usleep(20000);
	ShowRecSerStrings_ERR(6, 0x82, 0x1210, "175kWh");
	usleep(20000);

	//
	ShowRecSerStrings_ERR(12, 0x82, 0x1220, "CHARG_SET_P:");
	usleep(20000);

	//ShowRecSerStrings_ERR(4, 0x82, 0x1230, "--KW");
	ShowRecSerValue(0x82, 0x2570, SYS_SET_Charg_Power);
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1240, "W&L:");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1250, "--KW");
	usleep(20000);

	//
	ShowRecSerStrings_ERR(4, 0x82, 0x1260, "DEG:");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1270, "--KW");
	usleep(20000);
	ShowRecSerStrings_ERR(11, 0x82, 0x1280, "GRID_MAX_P:");
	usleep(20000);
	ShowRecSerStrings_ERR(6, 0x82, 0x1290, "66KW");
	usleep(20000);
	usleep(20000);

	//
}


//
extern volatile unsigned char g_car_vin[17];

//
void BMS_sys_ini(void)
{
	uint8			i	= 0;

	char			str[12] =
		{
		0
		};
	char			str1[12] =
		{
		0
		};


	//MAX12×Ö·û
	ShowRecSerStrings_ERR(12, 0x82, 0x1120, "BMS_VIN(10):");
	usleep(20000);

	for (i = 0; i < 10; i++)
		{
		str[i]				= g_car_vin[i];
		}

	str[10] 			= ' ';
	str[11] 			= ' ';

	ShowRecSerStrings_ERR(12, 0x82, 0x1130, str);
	usleep(20000);
	ShowRecSerStrings_ERR(11, 0x82, 0x1140, "BMS_VIN(7):");
	usleep(20000);

	for (i = 0; i < 7; i++)
		{
		str1[i] 			= g_car_vin[i + 10];
		}

	str1[7] 			= ' ';
	str1[8] 			= ' ';
	str1[9] 			= ' ';
	str1[10]			= ' ';
	str1[11]			= ' ';

	ShowRecSerStrings_ERR(12, 0x82, 0x1150, str1);
	usleep(20000);

	//MAX12×Ö·û
	ShowRecSerStrings_ERR(9, 0x82, 0x1160, "PACK_Num:");
	usleep(20000);
	ShowRecSerStrings_ERR(1, 0x82, 0x1170, "1");
	usleep(20000);
	ShowRecSerStrings_ERR(12, 0x82, 0x1180, "PACK_Hotsys:");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1190, "Cool");
	usleep(20000);

	//
	ShowRecSerStrings_ERR(9, 0x82, 0x11A0, "Cell_Num:");
	usleep(20000);
	ShowRecSerStrings_ERR(3, 0x82, 0x11B0, "180");
	usleep(20000);
	ShowRecSerStrings_ERR(9, 0x82, 0x11C0, "Temp_Num:");
	usleep(20000);
	ShowRecSerStrings_ERR(2, 0x82, 0x11D0, "45");
	usleep(20000);

	//
	ShowRecSerStrings_ERR(4, 0x82, 0x11E0, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x11F0, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1200, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1210, "----");
	usleep(20000);

	//
	ShowRecSerStrings_ERR(4, 0x82, 0x1220, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1230, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1240, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1250, "----");
	usleep(20000);

	//
	ShowRecSerStrings_ERR(4, 0x82, 0x1260, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1270, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1280, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1290, "----");
	usleep(20000);
	usleep(20000);

	//
}


void SYS_sys_ini(void)
{
	uint8			i	= 0;

	char			str[10] =
		{
		0
		};
	char			str1[7] =
		{
		0
		};


	//MAX12×Ö·û
	ShowRecSerStrings_ERR(12, 0x82, 0x1120, "EV_ChagreHW:");
	usleep(20000);
	ShowRecSerStrings_ERR(12, 0x82, 0x1130, "EVHW-V240120");
	usleep(20000);
	ShowRecSerStrings_ERR(12, 0x82, 0x1140, "EV_ChagreSW:");
	usleep(20000);
	ShowRecSerStrings_ERR(12, 0x82, 0x1150, "EVSW-V240512");
	usleep(20000);


	//MAX12×Ö·û
	ShowRecSerStrings_ERR(11, 0x82, 0x1160, "EV_ChagrPW:");
	usleep(20000);
	ShowRecSerStrings_ERR(5, 0x82, 0x1170, "40KW");
	usleep(20000);
	ShowRecSerStrings_ERR(9, 0x82, 0x1180, "EV_Max_V:");
	usleep(20000);
	ShowRecSerStrings_ERR(5, 0x82, 0x1190, "1000V");
	usleep(20000);

	//
	ShowRecSerStrings_ERR(9, 0x82, 0x11A0, "EV_Max_I:");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x11B0, "135A");
	usleep(20000);
	ShowRecSerStrings_ERR(8, 0x82, 0x11C0, "EV_RMIP:");
	usleep(20000);
	ShowRecSerStrings_ERR(9, 0x82, 0x11D0, "C0.A8.0.4");
	usleep(20000);

	//
	ShowRecSerStrings_ERR(8, 0x82, 0x11E0, "EV_LNIP:");
	usleep(20000);
	ShowRecSerStrings_ERR(10, 0x82, 0x11F0, "C0.A8.1.E8");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1200, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1210, "----");
	usleep(20000);

	//
	ShowRecSerStrings_ERR(4, 0x82, 0x1220, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1230, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1240, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1250, "----");
	usleep(20000);

	//
	ShowRecSerStrings_ERR(4, 0x82, 0x1260, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1270, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1280, "----");
	usleep(20000);
	ShowRecSerStrings_ERR(4, 0x82, 0x1290, "----");
	usleep(20000);
	usleep(20000);

	//
}


void HMI_LED_60s_OFF(void)
{
	//
	ShowRecSerValue2(0x82, 0x0082, 0x64641770);
	usleep(500000);

	//
}



void HMI_LED_60s(void)
{
	//
	ShowRecSerValue2(0x82, 0x0082, 0x64001770);
	usleep(500000);

	//
}


///////////////////////////////////////////////////////////////////////
uint16			dubg_SOC = 0;
uint8			gAA_falg = 0;
uint8			gAA_falg1 = 0;
uint8			gAA_falg2 = 0;



//															
int serial_main_HMI(void)
{

	char *			dev = "/dev/ttymxc1";

	//
	qsize			size = 0;
	uint8			iFor;
	uint32			timer_tick_last_update[100] =
		{
		0
		};

	//ÉÏÒ»´Î¸üÐÂµÄÊ±¼ä
	//
	signal(SIGINT, SignHandler);

	//dev	= argv[1];
	if (dev == NULL)
		{
		printf("Please input serial device name ,for exmaple /dev/ttymxc1.\n");
		exit(1);
		}

	fd					= OpenDev(dev);

	if (fd > 0)
		{
		}
	else 
		{

		printf("Can't Open Serial Port %s \n", dev);
		exit(0);

		}

	printf("\nWelcome to HMI-dwin! \n\n");

	pthread_mutex_init(&mut, NULL);

	if (start_thread_func(com_read, (pthread_t *) &thread[READ_THREAD_ID], (int *) &COM_READ_STATU, (int *) &COM_READ_STATU) !=
		 0)
		{
		printf("error to read leave\n");
		return - 1;
		}

	if (start_thread_func(com_send, (pthread_t *) &thread[SEND_THREAD_ID], (int *) &COM_SEND_STATU, (int *) &COM_SEND_STATU) !=
		 0)
		{
		printf("error to send leave\n");
		return - 1;
		}

	//

	/*Çå¿Õ´®¿Ú½ÓÊÕ»º³åÇø*/
	queue_reset();

	//
	HMI_LED_60s_OFF();
	delay_ms(100);
	HMI_LED_60s_OFF();
	delay_ms(100);
	HMI_LED_60s_OFF();

	//
	UpdateUI_Page64();


	UpdateUI_Page65();

	UpdateUI_Page70();


	UpdateUI_Page71();


	UpdateUI_Page72();


	UpdateUI_Page73_74_bms();

	//UpdateUI_Page73_74_bms();
	UpdateUI_Page75();


	UpdateUI_Page78();

	UpdateUI_Page81();


	UpdateUI_Page82();


	UpdateUI_Page84();


	UpdateUI_Page85();


	UpdateUI_Page95();

	UpdateUI_Page96();


	UpdateUI_Page97();


	UpdateUI_Page98();

	UpdateUI_Page99();

	delay_ms(100);

	//
	//ShowRecSerStrings(0x82, 0x1450, "STOP");
	//usleep(20000);

	/*ÇÐ»»Ò³ÃæÏÔÊ¾*/
	SetScreen(62);
	delay_ms(100);
	SetScreen(63);
	delay_ms(100);
	SetScreen(64);
	delay_ms(100);
	ShowRecSerValue(0x82, 0x9C03, 0xFFFF);			//ºìÉ« ShowRecSerValue(82,0x9A03,0xFFFF);//°×É«
	usleep(30000);
	ShowRecSerStrings_ERR(24, 0x82, 0x8003, "Energy Storage System!!!");
	usleep(80000);

	//
	Page64				= 1;
	SmartEnerg_io.OUT_2 = 0;
	gBMSInfo.Cell_show	= 0;

	//
	dubg_SOC			= 4000;

	//B2408
	UpdateSetcur_B2408_ESS(0);

	//
	UpdateSetcur_AC_CCS_MD(0);
	UpdateSetcur_AC_MD_ESS(0);
	UpdateSetcur_DC_CCS_ESS(0);
	UpdateSetcur_2408_ESS_PCS(0);
	UpdateSetcur_PCS_LD(0);
	UpdateSetcur_B2408_ccs(0);
	UpdateSetcur_B2408_OBD(0);

	//
	UpdateSetcur_B2408_CHARGE(0);
	UpdateSetcur_B2408_DISCG(0);
	UpdateSetcur_B2408_PCS(0);
	UpdateSetcur_B2408_LD(0);
	UpdateSetcur_B2408_ess1(1);

	while (1)
		{
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		timer_tick_count_[21] ++;

		if ((timer_tick_count_[21] >= 21) && (timer_tick_count_[21] < 25))
			{
			//timer_tick_count_[21] = 30;
			HMI_LED_60s();
			}

		if (timer_tick_count_[21] >= 25)
			timer_tick_count_[21] = 25;

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (my__export == 255)
			{
			my__export			= 0;
			SetScreen(116);
			delay_ms(5000);
			SetScreen(82);
			}

		if (my__export == 254)
			{
			my__export			= 0;
			SetScreen(116);
			delay_ms(5000);
			SetScreen(84);
			}

		if (my__export == 253)
			{
			my__export			= 0;
			SetScreen(116);
			delay_ms(5000);
			SetScreen(85);
			}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		//¼±Í£
		if (SmartEnerg_io.IN_1)
			{
			//UpdateSetCon_AA(2);
			//UpdateSetLoad_AA(2);
			//UpdateSetGrid_AA(2);
			//UpdateSetEss_AA(2, gBMSInfo.SOC, 0);
			//
			//UpdateSetcur3_L(0);
			//UpdateSetcur2_L(0);
			//UpdateSetcur1_L(0);
			//UpdateSetcur3_R(0);
			//UpdateSetcur2_R(0);
			//UpdateSetcur1_R(0);
			//
			//UpdateSetCon_WL(2);
			//
			//UpdateSetcur4_L(0);
			//UpdateSetcur5_R(0);
			//UpdateSetcur6_R(0);
			//UpdateSetcur7_L(0);
			//
			UpdateSetcur_B2408_EER(1);

			//
			SmartEnerg_io.OUT_4 = 1;
			SmartEnerg_io.OUT_1 = 0;
			SmartEnerg_io.OUT_3 = 0;

			//
			SmartEnerg_sta		= SmartEnergy_end;
			SmartEnerg_PGsta	= SmartEnergy_PGend;

			//
			gSmartEnerg_sys.SmartEnergy_gridend = 0;
			gSmartEnerg_sys.SmartEnergy_gridrun = 0;
			gSmartEnerg_sys.SmartEnergy_essend = 0;
			gSmartEnerg_sys.SmartEnergy_essrun = 0;

			gSmartEnerg_PGsys.SmartEnergy_WLchager = 0;
			gSmartEnerg_PGsys.SmartEnergy_DEGchager = 0;
			gSmartEnerg_PGsys.SmartEnergy_WLchager_end = 0;
			gSmartEnerg_PGsys.SmartEnergy_DEGchager_end = 0;

			//
			}

		//
		//
		//¹ö¶¯À¸ÐÅÏ¢Êä³ö£º
		if ((gBMSInfo.System_error != 0) || (gBMSInfo.Protection.all != 0))
			{
			//printf("%04x: ", gBMSInfo.System_error);
			//if ((gBMSInfo.System_error & 0x80) == 0x80)
				{
				ShowRecSerValue(0x82, 0x9C03, 0xF800); //ºìÉ« ShowRecSerValue(82,0x9A03,0xFFFF);//°×É«
				usleep(30000);
				ShowRecSerStrings_ERR(69, 0x82, 0x8003, "Discharge power overload shutdown, please remove the load to restart.");
				usleep(80000);
				}

			//UpdateSetCon_AA(2);
			//UpdateSetLoad_AA(2);
			//UpdateSetGrid_AA(2);
			//UpdateSetEss_AA(2, gBMSInfo.SOC, 0);
			//UpdateSetCon_WL(2);
			UpdateSetcur_B2408_EER(1);

			//
			SmartEnerg_io.OUT_4 = 1;

			SmartEnerg_io.OUT_1 = 0;
			SmartEnerg_io.OUT_3 = 0;

			//
			}

		else 
			{
			if (gBMSInfo.SOC < 20)
				{
				ShowRecSerValue(0x82, 0x9C03, 0xF800); //ºìÉ« ShowRecSerValue(82,0x9A03,0xFFFF);//°×É«
				usleep(30000);
				ShowRecSerStrings_ERR(60, 0x82, 0x8003, "Battery capacity is low, need to charge as soon as possible.");
				usleep(80000);

				//ShowRecSerValue(0x82, 0x9A03, 0xF800); //ºìÉ« ShowRecSerValue(82,0x9A03,0xFFFF);//°×É«
				//usleep(80000);
				}
			else 
				{
				ShowRecSerValue(0x82, 0x9C03, 0xFFFF); //ºìÉ« ShowRecSerValue(82,0x9A03,0xFFFF);//°×É«
				usleep(30000);
				ShowRecSerStrings_ERR(22, 0x82, 0x8003, "Energy Storage System!");
				usleep(80000);

				//ShowRecSerValue(0x82, 0x9A03, 0x0408); //ºìÉ« ShowRecSerValue(82,0x9A03,0xFFFF);//°×É«
				//usleep(80000);
				}
			}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

		/*
		if (SmartEnerg_io.IN_2 == 1)//ÊÖ¶¯¿ª¹Ø
			{
			//
			SmartEnerg_io.OUT_1 = 0;

			//
			UpdateSetGrid_AA(0);
			UpdateSetcur3_L(0);

			//UpdateSetcur3_R(0);
			//
			SmartEnerg_sta		= SmartEnergy_gridend;
			SmartEnerg_PGsta	= SmartEnergy_WLchager_end;
			}
		*/
		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (AC_Discharge_start_isOk != 0)
			{
			if ((SmartEnerg_sta == SmartEnergy_dischag_pcs_gird) ||
				 (SmartEnerg_sta == SmartEnergy_dischag_pcs_load) || (SmartEnerg_sta == SmartEnergy_chager_pcs))
				{
				AC_Discharge_start_isOk = 0;
				}
			else 
				{
				SmartEnerg_sta		= SmartEnergy_dischager;

				//SmartEnerg_sta		= SmartEnergy_essrun;
				//gSmartEnerg_sys.SmartEnergy_end = 1;
				//SmartEnerg_io.OUT_3 = 1;
				//
				AC_Discharge_start_isOk = 0;
				AC_Charge_start_isOk = 0;
				AC_Charge_stop_isOk = 0;
				}
			}

		if (AC_Discharge_stop_isOk != 0)
			{
			if (SmartEnerg_sta == SmartEnergy_chager_pcs)
				{
				AC_Discharge_stop_isOk = 0;
				}
			else 
				{
				if ((SmartEnerg_sta == SmartEnergy_dischag_pcs_gird) || (SmartEnerg_sta == SmartEnergy_dischag_pcs) ||
					 (SmartEnerg_sta == SmartEnergy_dischag_pcs_load)) //if (1)
					{
					AC_Discharge_stop_isOk = 0;

					SmartEnerg_sta		= SmartEnergy_dischag_end;

					//SmartEnerg_sta		= SmartEnergy_essend;
					}
				else 
					{
					//gSmartEnerg_sys.SmartEnergy_end = 1;
					//
					//SmartEnerg_sta		= SmartEnergy_dischag_end;//
					//
					AC_Discharge_start_isOk = 0;
					AC_Charge_start_isOk = 0;
					AC_Charge_stop_isOk = 0;
					}

				//AC_Discharge_stop_isOk = 0;
				AC_Discharge_start_isOk = 0;
				AC_Charge_start_isOk = 0;
				AC_Charge_stop_isOk = 0;

				//
				//SmartEnerg_io.OUT_3 = 0;
				//
				}

			}

		if (AC_Charge_start_isOk != 0) //if ((AC_Charge_start_isOk != 0) && (SmartEnerg_io.IN_2 == 0))
			{
			if ((SmartEnerg_sta == SmartEnergy_chager_pcs) || ((SmartEnerg_sta == SmartEnergy_dischag_pcs_gird) ||
				 (SmartEnerg_sta == SmartEnergy_dischag_pcs) || (SmartEnerg_sta == SmartEnergy_dischag_pcs_load)))
				{
				AC_Charge_start_isOk = 0;
				}
			else 
				{
				//SmartEnerg_sta		= SmartEnergy_chager;//B2408
				//SmartEnerg_sta		= SmartEnergy_gridrun;
				SmartEnerg_io.OUT_1 = 1;

				//gSmartEnerg_sys.SmartEnergy_end = 1;
				//
				AC_Discharge_stop_isOk = 0;
				AC_Discharge_start_isOk = 0;
				AC_Charge_start_isOk = 0;
				}

			//B2408
			CDZ_charge_start	= 1;
			AC_Charge_start_isOk = 0;
			}

		if (AC_Charge_stop_isOk != 0)
			{

			//
			if ((SmartEnerg_sta == SmartEnergy_dischag_pcs) || (SmartEnerg_sta == SmartEnergy_dischag_pcs_gird) ||
				 (SmartEnerg_sta == SmartEnergy_dischag_pcs_load))
				{
				AC_Charge_stop_isOk = 0;
				}
			else 
				{
				if (SmartEnerg_sta == SmartEnergy_chager_pcs) //if (1)
					{
					AC_Charge_stop_isOk = 0;

					SmartEnerg_sta		= SmartEnergy_chager_end;

					//SmartEnerg_sta		= SmartEnergy_gridend;
					}
				else 
					{
					//gSmartEnerg_sys.SmartEnergy_end = 1;
					//
					AC_Discharge_start_isOk = 0;
					AC_Charge_start_isOk = 0;
					AC_Discharge_stop_isOk = 0;
					}

				AC_Discharge_start_isOk = 0;
				AC_Charge_start_isOk = 0;
				AC_Discharge_stop_isOk = 0;
				}

			//SmartEnerg_io.OUT_1 = 0;
			//B2408
			//SmartEnerg_sta		= SmartEnergy_chager_end;
			CC2_DC_Run			= 2;				//B2408

			//if (CCS_AC_DC == 2)
				{
				//
				//CC2_DC_Run			= 2;			//B2408
				CCS_AC_DC			= 0;
				CDZ_charge_start	= 0;
				}

			AC_Charge_stop_isOk = 0;
			}


		///////////////////////////////////////////////////////////PG///////////////////////////////////////////////////////////////////
		if (WL_Charge_start_isOk != 0)
			{
			if ((SmartEnerg_PGsta == SmartEnergy_WLchager_pcs_gird) ||
				 (SmartEnerg_PGsta == SmartEnergy_WLchager_pcs_load))
				WL_Charge_start_isOk = 0;
			else 
				{
				SmartEnerg_PGsta	= SmartEnergy_WLchager;

				WL_Charge_start_isOk = 0;
				}
			}

		if (WL_Charge_stop_isOk != 0)
			{
			//if (SmartEnerg_sta == SmartEnergy_chager_pcs)
			if (1)
				{
				WL_Charge_stop_isOk = 0;

				SmartEnerg_PGsta	= SmartEnergy_WLchager_end;
				}

			}

		//
		if (DEG_charge_start_isOk != 0)
			{
			if ((SmartEnerg_PGsta == SmartEnergy_DEGchager_pcs_gird) ||
				 (SmartEnerg_PGsta == SmartEnergy_DEGchager_pcs_load))
				DEG_charge_start_isOk = 0;
			else 
				{
				SmartEnerg_PGsta	= SmartEnergy_DEGchager;

				DEG_charge_start_isOk = 0;
				}
			}

		if (DEG_charge_stop_isOk != 0)
			{
			//if (SmartEnerg_sta == SmartEnergy_chager_pcs)
			if (1)
				{
				DEG_charge_stop_isOk = 0;

				SmartEnerg_PGsta	= SmartEnergy_DEGchager_end;
				}

			}

		//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
		if (Page63)
			{
			Page64				= 1;
			}

		if (Page64)
			{

			UpdateUI_Page64();
			}

		//
		//timer_tick_count_[20] ++;
		if (timer_tick_count_[10] >= 5)
			{
			//timer_tick_count_[10] = 0;
			if (SmartEnerg_io.IN_3)
				{
				//UpdateSetcur4_L(1);
				}
			else 
				{
				//UpdateSetcur4_L(0);
				}

			//
			if (SmartEnerg_io.IN_1)
				{
				//UpdateSetCon_AA(2);
				//UpdateSetLoad_AA(2);
				//UpdateSetGrid_AA(2);
				//UpdateSetEss_AA(2, gBMSInfo.SOC, 0);
				//UpdateSetCon_WL(2);
				//
				//UpdateSetcur3_L(0);
				//UpdateSetcur2_L(0);
				//UpdateSetcur1_L(0);
				//
				UpdateSetcur_B2408_EER(1);

				//UpdateSetcur3_R(0);
				//UpdateSetcur2_R(0);
				//UpdateSetcur1_R(0);
				//
				SmartEnerg_io.OUT_4 = 1;
				SmartEnerg_io.OUT_1 = 0;
				SmartEnerg_io.OUT_3 = 0;

				//
				SmartEnerg_sta		= SmartEnergy_end;
				}
			else 
				{
				//
				//SmartEnerg_sta		= 0xFF;
				//
				if (gBMSInfo.System_error == 0)
					{
					if ((gAMT_Info[0].PMAPhaseCur.all == 0) && (gAMT_Info[0].PMBPhaseCur.all) &&
						 (gAMT_Info[0].PMCPhaseCur.all))
						{
						//UpdateSetcur3_L(0);
						//UpdateSetGrid_AA(0);
						}
					else 
						{
						//UpdateSetGrid_AA(1);
						if ((SmartEnerg_io.OUT_1) || (CDZ_charge_start))
							{
							//UpdateSetcur3_L(1);
							}
						else 
							{
							//UpdateSetGrid_AA(0);
							//UpdateSetcur3_L(0);
							}
						}

					/////////////////////////////
					if ((gAMT_Info[1].PMAPhaseCur.all == 0) && (gAMT_Info[2].PMAPhaseCur.all) &&
						 (gAMT_Info[3].PMAPhaseCur.all))
						{
						//UpdateSetLoad_AA(0);
						}
					else 
						{
						//UpdateSetLoad_AA(1);
						}

					/////////////////////////////
					//if (MonitorAck_BEG[0].Module_AC_AllP.all == 0)
					if (gAMT_Info[6].PMAPhaseVolt.all <= 1500)
						{
						PMMsgPars[6].PMVoltDataA.all = 0;
						PMMsgPars[6].PMVoltDataB.all = 0;
						PMMsgPars[6].PMVoltDataC.all = 0;
						PMMsgPars[6].PMCurDataA.all = 0;
						PMMsgPars[6].PMCurDataB.all = 0;
						PMMsgPars[6].PMCurDataC.all = 0;
						PMMsgPars[6].PMActivePowerData.all = 0;

						//
						//UpdateSetCon_AA(0);
						//UpdateSetcur2_L(0);
						}
					else 
						{
						//UpdateSetCon_AA(1);
						if (SmartEnerg_io.OUT_1)
							{
							//UpdateSetcur2_L(1);
							}
						else 
							{
							//UpdateSetcur2_R(1);
							}
						}

					if (gBMSInfo.BMSCur.all == 30000) //µçÁ÷Îª0
						{
						//UpdateSetEss_AA(0, gBMSInfo.SOC, 0);
						//UpdateSetcur1_L(0);
						//SmartEnerg_io.OUT_3 = 0;
						//SmartEnerg_io.OUT_1 = 0;
						//
						UpdateSetcur_B2408_ESS(0);

						//
						UpdateSetcur_AC_CCS_MD(0);
						UpdateSetcur_AC_MD_ESS(0);
						UpdateSetcur_DC_CCS_ESS(0);
						UpdateSetcur_2408_ESS_PCS(0);
						UpdateSetcur_PCS_LD(0);
						UpdateSetcur_B2408_ccs(0);
						UpdateSetcur_B2408_OBD(0);

						//
						UpdateSetcur_B2408_CHARGE(0);
						UpdateSetcur_B2408_DISCG(0);
						UpdateSetcur_B2408_PCS(0);
						UpdateSetcur_B2408_LD(0);
						}
					else 
						{
						//if (gSmartEnerg_sys.SmartEnergy_essrun)
						if (1)
							{
							//UpdateSetEss_AA(1, gBMSInfo.SOC, 0);
							UpdateSetcur_B2408_ESS(1);

							//if (SmartEnerg_io.OUT_1)
							//	UpdateSetcur1_L(1);
							//else 
							//	UpdateSetcur1_R(1);
							if (gBMSInfo.BMSCur.all < 30000) //B2401
								{
								//SmartEnerg_io.OUT_3 = 1;
								//SmartEnerg_io.OUT_2 = 0;
								//UpdateSetcur1_R(1);
								//
								UpdateSetcur_B2408_CHARGE(0);
								UpdateSetcur_B2408_DISCG(1);

								//
								UpdateSetcur_B2408_PCS(1);
								UpdateSetcur_B2408_LD(1);

								//
								UpdateSetcur_2408_ESS_PCS(1);
								UpdateSetcur_PCS_LD(1);

								//
								UpdateSetcur_AC_CCS_MD(0);
								UpdateSetcur_AC_MD_ESS(0);
								UpdateSetcur_DC_CCS_ESS(0);
								UpdateSetcur_B2408_ccs(0);
								UpdateSetcur_B2408_OBD(0);
								}
							else 
								{
								//SmartEnerg_io.OUT_3 = 0;
								//SmartEnerg_io.OUT_2 = 1;
								//UpdateSetcur1_L(1);
								//UpdateSetcur3_L(1);
								//
								UpdateSetcur_B2408_CHARGE(1);
								UpdateSetcur_B2408_DISCG(0);

								//
								UpdateSetcur_B2408_PCS(0);
								UpdateSetcur_B2408_LD(0);
								UpdateSetcur_2408_ESS_PCS(0);
								UpdateSetcur_PCS_LD(0);

								if (CF_Secc_DCAC_ChgMode == 0)
									{
									UpdateSetcur_AC_CCS_MD(0);
									UpdateSetcur_AC_MD_ESS(0);
									UpdateSetcur_DC_CCS_ESS(0);
									UpdateSetcur_B2408_ccs(0);
									UpdateSetcur_B2408_OBD(0);

									}
								else 
									{
									UpdateSetcur_B2408_ccs(1);

									if (CF_Secc_DCAC_ChgMode == 1)
										{
										UpdateSetcur_DC_CCS_ESS(1);
										UpdateSetcur_AC_CCS_MD(0);
										UpdateSetcur_AC_MD_ESS(0);

										//
										UpdateSetcur_B2408_OBD(0);
										}
									else 
										{
										UpdateSetcur_AC_CCS_MD(1);
										UpdateSetcur_AC_MD_ESS(1);
										UpdateSetcur_DC_CCS_ESS(0);

										//
										UpdateSetcur_B2408_OBD(1);
										}
									}
								}
							}
						else 
							{
							//UpdateSetEss_AA(0, gBMSInfo.SOC, 0);
							//UpdateSetcur1_L(0);
							UpdateSetcur_B2408_PCS(0);
							UpdateSetcur_B2408_LD(0);
							UpdateSetcur_2408_ESS_PCS(0);
							UpdateSetcur_PCS_LD(0);

							//
							//SmartEnerg_io.OUT_3 = 0;
							//SmartEnerg_io.OUT_2 = 0;
							}
						}
					}
				}

			timer_tick_count_[10] = 0;
			}

		if (Page65)
			{
			UpdateUI_Page65();
			}

		//
		//timer_tick_count_[9] ++;
		if (timer_tick_count_[9] >= 5)
			{
			//timer_tick_count_[9] = 0;
			if (SmartEnerg_io.IN_2)
				{
				//UpdateSetCon_WL(1);
				}
			else 
				{
				//UpdateSetCon_WL(0);
				}

			//ÅÐ¶ÏPVµçÁ÷ µçÑ¹
			if ((gWL_Info.LCur.all == 0) || (gWL_Info.LVolt.all == 0))
				{
				UpdateSetcur5_R(0);
				UpdateSetcur6_R(0);
				}
			else 
				{
				//UpdateSetcur5_R(1);
				//UpdateSetcur6_R(1);
				}

			//ÅÐ¶ÏDEGµçÁ÷
			if (gMonitorPG[0].Module_AC_ACur.all == 0)
				{
				UpdateSetcur7_L(0);
				}
			else 
				{
				//UpdateSetcur7_L(1);
				}

			timer_tick_count_[9] = 0;
			}


		if (Page70)
			{
			UpdateUI_Page70();

			}

		if (Page71)
			{
			UpdateUI_Page71();

			}

		if (Page113)
			{
			UpdateUI_Page113();

			}

		if (Page114)
			{
			UpdateUI_Page114();

			}

		if (Page72)
			{
			UpdateUI_Page72();

			}

		if (Page73)
			{
			UpdateUI_Page73_74_bms();
			gBMSInfo.Cell_show	= 1;
			}

		if (Page74)
			{
			UpdateUI_Page73_74_bms();
			gBMSInfo.Cell_show	= 1;

			}

		if (Page75)
			{
			UpdateUI_Page75();

			}

		if (Page78)
			{
			UpdateUI_Page78();

			}

		if (Page81)
			{
			UpdateUI_Page81();

			}

		if (Page82)
			{
			UpdateUI_Page82();

			}

		if (Page84)
			{
			UpdateUI_Page84();

			}

		if (Page85)
			{
			UpdateUI_Page85();

			}

		if (Page95)
			{
			UpdateUI_Page95();

			}

		if (Page96)
			{
			UpdateUI_Page96();

			}

		if (Page97)
			{
			UpdateUI_Page97();

			}

		if (Page98)
			{
			UpdateUI_Page98();

			}

		if (Page99)
			{
			UpdateUI_Page99();

			}

		//
		if (Page204_B2408)
			{
			Page204_B2408_relay();

			}

		if ((Page73 == 0) && (Page74 == 0))
			gBMSInfo.Cell_show = 0;

		//
		dubg_SOC++;

		if (dubg_SOC >= 3600)
			{
			EMSReadRTC();
			dubg_SOC			= 0;
			}

		//	dubg_SOC = 0;
		//if (SmartEnerg_io.IN_1) UpdateSetEss_AA(2, dubg_SOC, 0);

		/*//
		//ÄÜÁ¿¹ÜÀíÊý¾Ý¿â
		SmartEnergy_Status_DB();

		//
		usleep(100000);

		//
		SmartEnergy_time_DB();
		usleep(100000);
		SmartEnergy_faul_DB();
		usleep(100000);
		*/
		//
		printf("\nSmartEnerg_sta: %d \n", SmartEnerg_sta);
		usleep(100000);
		printf("\SmartEnerg_PGsta: %d \n", SmartEnerg_PGsta);

		/*
				//BWTÊý¾Ý¿â
				switch (BWT_DB_Flg)
					{
					case 0:
						BWT_DB_Flg = 0;
						break;

					case 1:
						gbtbox_DB();
						BWT_DB_Flg = 0;
						break;

					case 2:
						BWT_DB_Flg = 0;
						break;

					case 3:
						gb_DB_set();
						BWT_DB_Flg = 0;
						break;

					default:
						BWT_DB_Flg = 0;
						break;
					}

				//
		*/
		//usleep(100000);
		}

	sleep(10);

	//reboot(RB_AUTOBOOT);
	printf("\nHMI-dwin down.\n");
	pthread_exit(NULL);
	return 0;

}


/*!
 *	\brief	ÏûÏ¢´¦ÀíÁ÷³Ì
 *	\param msg ´ý´¦ÀíÏûÏ¢
 *	\param size ÏûÏ¢³¤¶È
 */
//
unsigned char	Query_info_sta = 0;
unsigned char	Query_info_BOM = 0;
unsigned char	Query_info_frist82 = 0;
unsigned char	Query_info_frist84 = 0;
unsigned char	Query_info_frist85 = 0;
unsigned char	Query_info_frist110 = 0;
unsigned char	Query_info_frist111 = 0;
unsigned char	Query_info_frist112 = 0;

//
void ProcessMessage(PCTRL_MSG msg, uint16 size)
{
	uint8			iFor;
	uint8			cmd_type;
	uint16			address;
	uint16			button;
	uint32			keyData[3];
	uint32			ReadReg[12];

	//
	char			tmp[128] =
		{
		0
		};

	//
	uint64_t		key_numtemp = 0;

	cmd_type			= msg->cmd_type;

	/*//
	printf("\n ProcessMessage \n");
	printf("\n %02X \n", msg->data_len);
	printf("\n %02X \n", msg->cmd_type);
	printf("\n %02X \n", msg->addr[0]);
	printf("\n %02X \n", msg->addr[1]);
	printf("\n %02X \n", msg->datalen);
	printf("\n %02X \n", msg->data[0]);
	printf("\n %02X \n", msg->data[1]);
	printf("\n %02X \n", msg->data[2]);
	printf("\n %02X \n", msg->data[3]);
	printf("\n %02X \n", msg->data[4]);
	printf("\n %02X \n", msg->data[5]);
	printf("\n %02X \n", msg->data[6]);
	printf("\n %02X \n", msg->data[7]);
	printf("\n ProcessMessage_end \n");
	*/
	//
	switch (cmd_type)
		{
		case 0x82:
				{
				/*****¶Á¼Ä´æÆ÷Öµ************/
				address 			= ((uint16) msg->addr[0]) << 8 | msg->addr[1];

				switch (address)
					{
					////////////////////////////////¹ö¶¯Ò³Ãæ»Ø¸´
					case 0x4f4b: //
							{
							if ((msg->data[1] == 0x82) || (msg->data[1] == 82))
								{
								Page_set_page();

								if (Query_info_frist82 == 0)
									{
									//
									Query_info_sta		= 0;
									Query_info_frist82	= 1;
									Query_info_frist84	= 0;
									Query_info_frist85	= 0;
									Query_info_frist110 = 0;
									Query_info_frist111 = 0;
									Query_info_frist112 = 0;

									//
									Page_Query_num		= 0;
									Enquiries_Page82	= 0;
									Enquiries_Page84	= 0;
									Enquiries_Page85	= 0;
									}

								//
								Page82				= 1;
								Query_info_BOM		= 1;

								//printf("Page84: %d \n", Page84);
								}

							if (msg->data[1] == 110)
								{
								Page_set_page();

								if (Query_info_frist110 == 0)
									{
									//
									Query_info_sta		= 0;
									Query_info_frist82	= 0;
									Query_info_frist84	= 0;
									Query_info_frist85	= 0;
									Query_info_frist110 = 1;
									Query_info_frist111 = 0;
									Query_info_frist112 = 0;

									//
									Page_Query_num		= 0;
									Enquiries_Page82	= 0;
									Enquiries_Page84	= 0;
									Enquiries_Page85	= 0;
									}

								//
								Page82				= 1;
								Query_info_BOM		= 2;

								//printf("Page84: %d \n", Page84);
								}

							if (msg->data[1] == 111)
								{
								Page_set_page();

								if (Query_info_frist111 == 0)
									{
									//
									Query_info_sta		= 0;
									Query_info_frist82	= 0;
									Query_info_frist84	= 0;
									Query_info_frist85	= 0;
									Query_info_frist110 = 0;
									Query_info_frist111 = 1;
									Query_info_frist112 = 0;

									//
									Page_Query_num		= 0;
									Enquiries_Page82	= 0;
									Enquiries_Page84	= 0;
									Enquiries_Page85	= 0;
									}

								//
								Page82				= 1;
								Query_info_BOM		= 3;

								//printf("Page84: %d \n", Page84);
								}

							////////////////////
							if (msg->data[1] == 84)
								{
								Page_set_page();

								if (Query_info_frist84 == 0)
									{
									//
									Query_info_sta		= 0;
									Query_info_frist82	= 0;
									Query_info_frist84	= 1;
									Query_info_frist85	= 0;
									Query_info_frist110 = 0;
									Query_info_frist111 = 0;
									Query_info_frist112 = 0;

									//
									Page_Query_num		= 0;
									Enquiries_Page82	= 0;
									Enquiries_Page84	= 0;
									Enquiries_Page85	= 0;
									}

								//
								Page84				= 1;

								//printf("Page84: %d \n", Page84);
								Query_info_BOM		= 1;
								}

							if (msg->data[1] == 112)
								{
								Page_set_page();

								if (Query_info_frist112 == 0)
									{
									//
									Query_info_sta		= 0;
									Query_info_frist82	= 0;
									Query_info_frist84	= 0;
									Query_info_frist85	= 0;
									Query_info_frist110 = 0;
									Query_info_frist111 = 0;
									Query_info_frist112 = 1;

									//
									Page_Query_num		= 0;
									Enquiries_Page82	= 0;
									Enquiries_Page84	= 0;
									Enquiries_Page85	= 0;
									}

								//
								Page84				= 1;

								//printf("Page84: %d \n", Page84);
								Query_info_BOM		= 2;
								}

							//////////////////
							if (msg->data[1] == 85)
								{
								Page_set_page();

								if (Query_info_frist85 == 0)
									{
									//
									Query_info_sta		= 0;
									Query_info_frist82	= 0;
									Query_info_frist84	= 0;
									Query_info_frist85	= 1;
									Query_info_frist110 = 0;
									Query_info_frist111 = 0;
									Query_info_frist112 = 0;

									//
									Page_Query_num		= 0;
									Enquiries_Page82	= 0;
									Enquiries_Page84	= 0;
									Enquiries_Page85	= 0;
									}

								//
								Page85				= 1;

								//printf("Page84: %d \n", Page84);
								Query_info_BOM		= 1;
								}

							//////////////////
							}
						////////////////////////////////¹ö¶¯Ò³Ãæ»Ø¸´-end
						break;

					default:
						//Page_set_info();
						break;
					}

				break;
				}

		case 0x83:
				{
				button				= ((uint16) msg->addr[0]) << 8 | msg->addr[1]; //ÒÆÎ»Çó»ò±ä³É16Î»Êý¾Ý

				switch (button)
					{
					////////////Tmie¶ÁÈ¡ ¶¨Ê±Ë¢ÐÂµ×²ãÊ±¼ä
					case 0x0010:
						key_numtemp = (msg->data[2]) + (msg->data[1] *100) + (msg->data[0] *10000);
						date_setdata(key_numtemp);
						key_numtemp = (msg->data[6]) + (msg->data[5] *100) + (msg->data[4] *10000);
						time_setdata(key_numtemp);

						//snprintf(tmp, sizeof(tmp), "date -s %s", SYS_SET_time);
						sprintf(tmp, "date -s %s", SYS_SET_time);
						printf("SYS_SET_time:%s\n", tmp);
						system(tmp);
						usleep(500000);
						system("hwclock -w");
						usleep(500000);
						system("hwclock");
						break;

					////////////Charge	
					case 0x5002:
						if (msg->data[1] == 1)
							{ //AC discharge start OK
							AC_Charge_start_isOk = 1;

							//
							//SmartEnerg_io.OUT_1 = 1;
							//
							printf("\n AC_Charge_start_isOk = 1 \n");

							//
							CC2_DC_Run			= 1; //B2408
							CCS_AC_DC			= 2;

							//
							}
						else 
							{
							if (msg->data[1] == 2)
								{ //AC discharge start NO
								AC_Charge_start_isOk = 0;
								printf("\n AC_Charge_start_isOk = 0 \n");

								//
								CC2_DC_Run			= 1; //B2408
								CCS_AC_DC			= 1;

								//
								}
							}

						//
						if (msg->data[1] == 3)
							{ //AC discharge stop OK
							AC_Charge_stop_isOk = 1;

							//
							//SmartEnerg_io.OUT_1 = 0;
							//
							printf("\n AC_Charge_stop_isOk = 1 \n");
							}
						else 
							{
							if (msg->data[1] == 4)
								{ //AC discharge stop NO

								AC_Charge_stop_isOk = 0;
								printf("\n AC_Charge_stop_isOk = 0 \n");
								}
							}

						break;

					/////////////////discharge
					case 0x5001:
						if (msg->data[1] == 1)
							{ //AC discharge start OK
							AC_Discharge_start_isOk = 1;

							//
							//SmartEnerg_io.OUT_1 = 0;
							//
							printf("\n AC_Discharge_start_isOk = 1 \n");
							}
						else 
							{
							if (msg->data[1] == 2)
								{ //AC discharge start NO
								AC_Discharge_start_isOk = 0;

								//
								//SmartEnerg_io.OUT_1 = 0;
								//
								printf("\n AC_Discharge_start_isOk = 0 \n");
								}
							}

						//
						if (msg->data[1] == 3)
							{ //AC discharge stop OK
							AC_Discharge_stop_isOk = 1;

							//
							//SmartEnerg_io.OUT_1 = 0;
							//
							printf("\n AC_Discharge_stop_isOk = 1 \n");
							}
						else 
							{
							if (msg->data[1] == 4)
								{ //AC discharge stop NO
								AC_Discharge_stop_isOk = 0;

								//
								//SmartEnerg_io.OUT_1 = 0;
								//
								printf("\n AC_Discharge_stop_isOk = 0 \n");
								}
							}

						break;

					////////////W&L
					case 0x5003:
						if (msg->data[1] == 1)
							{ //AC discharge start OK
							WL_Charge_start_isOk = 1;
							printf("\n W&L Charge_start_isOk = 1 \n");
							}
						else 
							{
							if (msg->data[1] == 2)
								{ //AC discharge start NO
								WL_Charge_start_isOk = 0;
								printf("\n W&L Charge_start_isOk = 0 \n");
								}
							}

						//
						if (msg->data[1] == 3)
							{ //AC discharge stop OK
							WL_Charge_stop_isOk = 1;
							printf("\n W&L Charge_stop_isOk = 1 \n");
							}
						else 
							{
							if (msg->data[1] == 4)
								{ //AC discharge stop NO
								WL_Charge_stop_isOk = 0;
								printf("\n W&L Charge_stop_isOk = 0 \n");
								}
							}

						break;

					////////////DEG
					case 0x5004:
						if (msg->data[1] == 1)
							{ //AC discharge start OK
							DEG_charge_start_isOk = 1;
							printf("\n DEG_charge_start_isOk = 1 \n");
							}
						else 
							{
							if (msg->data[1] == 2)
								{ //AC discharge start NO
								DEG_charge_start_isOk = 0;
								printf("\n DEG_charge_start_isOk = 0 \n");
								}
							}

						//
						if (msg->data[1] == 3)
							{ //AC discharge stop OK
							DEG_charge_stop_isOk = 1;
							printf("\n DEG_charge_stop_isOk = 1 \n");
							}
						else 
							{
							if (msg->data[1] == 4)
								{ //AC discharge stop NO
								DEG_charge_stop_isOk = 0;
								printf("\n DEG_charge_stop_isOk = 0 \n");
								}
							}

						break;

					////////////¼üÅÌÊäÈë
					case 0x7000: //ÈÕÆÚ
						//if (msg->data[0] == 4)
							{ //
							key_numtemp 		=
								 msg->data[7] + (msg->data[6] *256) + (msg->data[5] *65536) + (msg->data[4] *16777216) + (msg->data[3] *4294967295);

							//printf("\ngkey_numtemp: %d \n", key_numtemp);
							//
							//
							date_setdata(key_numtemp);
							SYS_SET_time_flg	= 1;
							}
						break;

					case 0x7010: //Ê±¼ä
						//if (msg->data[0] == 4)
							{ //
							key_numtemp 		=
								 msg->data[7] + (msg->data[6] *256) + (msg->data[5] *65536) + (msg->data[4] *16777216) + (msg->data[3] *4294967295);

							//printf("\ngkey_numtemp: %d \n", key_numtemp);
							time_setdata(key_numtemp);

							if (SYS_SET_time_flg == 1)
								{
								SYS_SET_time_flg	= 2;

								//printf("SYS_SET_time:%s\n", SYS_SET_time);
								//
								//snprintf(tmp, sizeof(tmp), "date -s %s", SYS_SET_time);
								sprintf(tmp, "date -s %s", SYS_SET_time);

								//printf("SYS_SET_time:%s\n", tmp);
								system(tmp);
								usleep(500000);
								system("hwclock -w");

								//
								RTC_settime();

								//
								}

							}
						break;

					case 0x7020: //ÉèÖÃ³äµç¹¦ÂÊ
						//if (msg->data[0] == 4)
							{ //
							//printf("\n(msg->data[0]: %d \n", msg->data[0]);
							//printf("\n(msg->data[1]: %d \n", msg->data[1]);
							//printf("\n(msg->data[2]: %d \n", msg->data[2]);
							//printf("\n(msg->data[3]: %d \n", msg->data[3]);
							//printf("\n(msg->data[4]: %d \n", msg->data[4]);
							//printf("\n(msg->data[5]: %d \n", msg->data[5]);
							key_numtemp 		= msg->data[1] + (msg->data[0] *256);

							//printf("\ngkey_numtemp: %d \n", key_numtemp);
							//B2401
							if (key_numtemp <= 66)
								{
								SYS_SET_Charg_Power = key_numtemp;
								}
							else 
								{
								SYS_SET_Charg_Power = 66;
								}
							}

						//
						ShowRecSerValue(0x82, 0x2570, SYS_SET_Charg_Power);
						usleep(20000);
						break;

					case 0x7030: //ÉèÖÃPCSÄ£¿éÊýÁ¿
						//if (msg->data[0] == 4)
							{ //
							//printf("\n(msg->data[0]: %d \n", msg->data[0]);
							//printf("\n(msg->data[1]: %d \n", msg->data[1]);
							//printf("\n(msg->data[2]: %d \n", msg->data[2]);
							//printf("\n(msg->data[3]: %d \n", msg->data[3]);
							//printf("\n(msg->data[4]: %d \n", msg->data[4]);
							//printf("\n(msg->data[5]: %d \n", msg->data[5]);
							key_numtemp 		= msg->data[1] + (msg->data[0] *256);

							//printf("\ngkey_numtemp: %d \n", key_numtemp);
							//B2401
							if (key_numtemp <= 10)
								{
								SYS_SET_PCS_Num 	= key_numtemp;

								//
								if (SYS_SET_PCS_Num <= (PCS_DCModeNum - 1))
									{
									DCModeNum			= SYS_SET_PCS_Num + 1; //¼ÓÉÏÐéÄâµÄÒ»¸ö£¬ÕæÊµÄ£¿éÊýÁ¿+1
									}
								else 
									{
									DCModeNum			= PCS_DCModeNum;
									}

								//
								}
							else 
								{
								SYS_SET_PCS_Num 	= 3; //¼ÓÉÏÐéÄâµÄÒ»¸ö£¬ÕæÊµÄ£¿éÊýÁ¿+1
								DCModeNum			= PCS_DCModeNum;
								}
							}

						//
						ShowRecSerValue(0x82, 0x2580, SYS_SET_PCS_Num);
						usleep(20000);
						break;

					////////////¼üÅÌÊäÈë-end
					////////////Page ÇÐ»»
					case 0xA000:
						if (msg->data[1] == 0x63)
							{ //
							Page_ini();
							Page63				= 1;
							}

						if (msg->data[1] == 0x70)
							{ //
							Page_ini();
							Page70				= 1;
							Page64				= 0;

							//
							Chagre_once_KWH 	= 0;
							ShowRecSerValue2(0x82, 0x2580, (uint32) (0)); //µ±Ç°
							usleep(20000);
							DisChagre_once_KWH	= 0;
							ShowRecSerValue2(0x82, 0x25A0, (uint32) (0)); //µ±Ç°
							usleep(20000);
							}

						if (msg->data[1] == 0x71)
							{ //
							Page_ini();
							Page71				= 1;
							Page64				= 0;
							}

						if (msg->data[1] == 0x72)
							{ //
							Page_ini();
							Page72				= 1;
							Page64				= 0;
							}

						if (msg->data[1] == 0x73)
							{ //
							Page_ini();
							Page73				= 1;
							Page64				= 0;
							}

						if (msg->data[1] == 0x75)
							{ //
							Page_ini();
							Page75				= 1;
							Page64				= 0;
							}

						if (msg->data[1] == 0x65)
							{ //
							Page_ini();
							Page65				= 1;
							Page64				= 0;
							}

						if (msg->data[1] == 0xFE)
							{ //
							SmartEnerg_save 	= 1;
							}

						if (msg->data[1] == 0xFF)
							{ //
							SmartEnerg_save 	= 0;

							//Page_ini();
							UpdateSerRollerr();
							UpdateSetCon_AA(gAA_falg);
							UpdateSetLoad_AA(gAA_falg);
							UpdateSetGrid_AA(gAA_falg);

							//
							UpdateSetcur1_L(gAA_falg);
							UpdateSetcur2_L(gAA_falg);
							UpdateSetcur3_L(gAA_falg);

							//
							UpdateSetcur1_R(gAA_falg1);
							UpdateSetcur2_R(gAA_falg1);
							UpdateSetcur3_R(gAA_falg1);

							//
							UpdateSetcur4_L(gAA_falg1);
							UpdateSetcur5_R(gAA_falg1);
							UpdateSetcur6_R(gAA_falg1);
							UpdateSetcur7_L(gAA_falg1);

							//
							gAA_falg++;
							gAA_falg1++;

							//printf("gAA_falg: %d \n", gAA_falg);
							//printf("gAA_falg1: %d \n", gAA_falg1);
							if (gAA_falg >= 3)
								gAA_falg = 0;

							if (gAA_falg1 >= 3)
								gAA_falg1 = 0;

							//
							}

						if (msg->data[1] == 0x81)
							{ //
							Page_ini();
							Page_set_info();
							Page81				= 1;
							SYS_info_flg		= 1;
							Query_info_sta		= 0;
							}

						if (msg->data[1] == 0x82)
							{ //
							Page_ini();
							Page_set_info();
							Page82				= 1;
							SYS_info_flg		= 2;
							Query_info_sta		= 0;
							}

						break;

					case 0xA010:
						if (msg->data[1] == 0x1) //sys_set
							{ //

							if (SYS_SET_Page81 == 0)
								SYS_SET_Page81 = 0;
							else 
								SYS_SET_Page81--;

							Query_info_sta		= 0;

							}
						else 
							{

							if (msg->data[1] == 0x2)
								{ //

								if (SYS_SET_Page81 >= 5)
									SYS_SET_Page81 = 5;
								else 
									SYS_SET_Page81++;

								}

							Query_info_sta		= 0;
							}

						if (msg->data[1] == 0x3) //Enquiries
							{ //

							if (Page82 == 1)
								{
								if (Enquiries_Page82 == 0)
									Enquiries_Page82 = 0;
								else 
									Enquiries_Page82--;

								}

							if (Page84 == 1)
								{
								if (Enquiries_Page84 == 0)
									Enquiries_Page84 = 0;
								else 
									Enquiries_Page84--;

								}

							if (Page85 == 1)
								{
								if (Enquiries_Page85 == 0)
									Enquiries_Page85 = 0;
								else 
									Enquiries_Page85--;

								}

							Query_info_sta		= 0;

							}
						else 
							{

							if (msg->data[1] == 0x4) //×Ü¹²1000Ìõ
								{ //

								if (Page82 == 1)
									{

									if (Enquiries_Page82 >= 499)
										Enquiries_Page82 = 499;
									else 
										Enquiries_Page82++;

									}

								if (Page84 == 1)
									{

									if (Enquiries_Page84 >= 499)
										Enquiries_Page84 = 499;
									else 
										Enquiries_Page84++;

									}

								if (Page85 == 1)
									{

									if (Enquiries_Page85 >= 499)
										Enquiries_Page85 = 499;
									else 
										Enquiries_Page85++;

									}
								}

							Query_info_sta		= 0;
							}

						if (msg->data[1] == 0x5) //repoort save
							{ //

							//my_report_export();
							my__export			= 1;
							SetScreen(115);
							}

						if (msg->data[1] == 0x6) //save
							{ //

							//my_log_export();
							my__export			= 2;
							SetScreen(115);
							}

						if (msg->data[1] == 0x7) //save
							{ //

							//my_alarm_export();
							my__export			= 3;
							SetScreen(115);
							}

						if (msg->data[1] == 0x64)
							{ //
							Page_ini();
							Page_set_info();
							Page64				= 1;

							}

						if ((msg->data[0] == 0x1) && (msg->data[1] == 0x13)) //113Ò³Ãæ
							{ //
							Page_ini();
							Page_set_info();
							Page113 			= 1;

							}

						if ((msg->data[0] == 0x1) && (msg->data[1] == 0x14)) //114Ò³Ãæ
							{ //
							Page_ini();
							Page_set_info();
							Page114 			= 1;

							}

						if (msg->data[1] == 0x71)
							{ //
							Page_ini();
							Page_set_info();
							Page71				= 1;

							}

						if (msg->data[1] == 0x81)
							{ //
							Page_ini();
							Page_set_info();
							Page81				= 1;

							}

						if (msg->data[1] == 0x95)
							{ //
							Page_ini();
							Page_set_info();
							Page95				= 1;

							}

						if (msg->data[1] == 0x99)
							{ //
							Page_ini();
							Page_set_info();
							Page99				= 1;

							}

						break;

					case 0xA012:
						if (msg->data[1] == 0x73)
							{ //
							Page_ini();

							Page74				= 0;

							//
							Page73				= 1;

							}

						if (msg->data[1] == 0x74)
							{ //
							Page_ini();

							Page74				= 1;

							//
							Page73				= 0;

							}

						break;

					case 0xA020:
						if (msg->data[1] == 0x78)
							{ //
							Page_ini();

							Page65				= 0;

							//
							Page78				= 1;

							}

						if (msg->data[1] == 0x64)
							{ //
							Page_ini();

							Page64				= 1;

							//
							Page65				= 0;

							}

						if (msg->data[1] == 0x65)
							{ //
							Page_ini();

							Page78				= 0;

							//
							Page65				= 1;

							}

						break;

					case 0xA030: //chanrging
						if (msg->data[1] == 1)
							{ //

							if (CDZ_charge_start == 0)
								{
								CDZ_charge_start	= 1;
								CDZ_charge_stop 	= 0;

								//
								ShowRecSerStrings(0x82, 0x1450, "RUN ");
								usleep(20000);
								}

							}

						if (msg->data[1] == 3)
							{ //

								{
								if (CDZ_charge_stop == 0)
									{
									CDZ_charge_stop 	= 1;
									CDZ_charge_start	= 0;

									//
									ShowRecSerStrings(0x82, 0x1450, "STOP");
									usleep(20000);
									}
								}
							}

						break;

					case 0xA100: //2408-relay
						Page_ini();
						Page204_B2408 = 1;
						break;

					////////////Page µ¥Ìå
					case 0xA110: //2408
						Page_ini();
						Page73 = 1;
						break;

					case 0x6000:
						Page_bln();
						Page73_last = msg->data[1];
						UpdateUI_Page73(Page73_last);
						break;

					case 0x6001:
						Page_bln();
						Page73_next = msg->data[1];
						UpdateUI_Page73(Page73_next);
						break;

					case 0x6002:
						Page_bln();
						Page74_last = msg->data[1];
						UpdateUI_Page74(Page74_last);
						break;

					case 0x6003:
						Page_bln();
						Page74_next = msg->data[1];
						UpdateUI_Page74(Page74_next);
						break;

					default:
						//buttonFlag = 0x00;
						Page_ini();

						//Page_set_info();
						Page_bln();
						break;
					}

				break;
				}

		default:
			break;
		}
}


uint8_t HexToChar(uint8_t temp)
{
	uint8_t 		dst;

	if (temp < 10)
		{
		dst 				= temp + '0';
		}
	else 
		{
		dst 				= temp - 10 + 'A';
		}

	return dst;
}


/*************************¼üÅÌÊý¾Ý´¦Àí***********************************/
void date_setdata(uint64_t indata)
{
	char			str[8] =
		{
		0
		};
	str[0]				= HexToChar((uint8_t) (indata / 100000));
	indata				= indata % 100000;
	str[1]				= HexToChar((uint8_t) (indata / 10000));
	indata				= indata % 10000;
	str[2]				= '-';
	str[3]				= HexToChar((uint8_t) (indata / 1000));
	indata				= indata % 1000;
	str[4]				= HexToChar((uint8_t) (indata / 100));
	indata				= indata % 100;
	str[5]				= '-';
	str[6]				= HexToChar((uint8_t) (indata / 10));
	indata				= indata % 10;
	str[7]				= HexToChar((uint8_t) (indata));
	SYS_SET_time[0] 	= '2';
	SYS_SET_time[1] 	= '0';
	SYS_SET_time[2] 	= str[0];
	SYS_SET_time[3] 	= str[1];
	SYS_SET_time[4] 	= '.';
	SYS_SET_time[5] 	= str[3];
	SYS_SET_time[6] 	= str[4];
	SYS_SET_time[7] 	= '.';
	SYS_SET_time[8] 	= str[6];
	SYS_SET_time[9] 	= str[7];
	SYS_SET_time[10]	= '-';
	ShowRecSerStrings_ERR(8, 0x82, 0x1130, str);
	usleep(20000);

	//
}




void time_setdata(uint64_t indata)
{
	char			str[8] =
		{
		0
		};
	str[0]				= HexToChar((uint8_t) (indata / 100000));
	indata				= indata % 100000;
	str[1]				= HexToChar((uint8_t) (indata / 10000));
	indata				= indata % 10000;
	str[2]				= ':';
	str[3]				= HexToChar((uint8_t) (indata / 1000));
	indata				= indata % 1000;
	str[4]				= HexToChar((uint8_t) (indata / 100));
	indata				= indata % 100;
	str[5]				= ':';
	str[6]				= HexToChar((uint8_t) (indata / 10));
	indata				= indata % 10;
	str[7]				= HexToChar((uint8_t) (indata));
	SYS_SET_time[11]	= str[0];
	SYS_SET_time[12]	= str[1];
	SYS_SET_time[13]	= str[2];
	SYS_SET_time[14]	= str[3];
	SYS_SET_time[15]	= str[4];
	SYS_SET_time[16]	= str[5];
	SYS_SET_time[17]	= str[6];
	SYS_SET_time[18]	= str[7];
	ShowRecSerStrings_ERR(8, 0x82, 0x1150, str);
	usleep(20000);


}



/************************************************************/
/*!
 *	\brief	¸üÐÂÏà¹ØÊý¾ÝÄÚÈÝ
 *	\param ÐòÁÐºÅ
 */
void UpdateEsc(void)
{
	uchar			String[4];

	char			str[8] =
		{
		0
		};

	//
	//gBMSInfo.BMS_Sta	= 0x8f;
	String[0]			= gBMSInfo.Error >> 4;
	String[1]			= gBMSInfo.Error & 0xf;
	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	String[0]			= gBMSInfo.BMS_Sta >> 4;
	String[1]			= gBMSInfo.BMS_Sta & 0xf;
	str[2]				= HexToChar(String[0]);
	str[3]				= HexToChar(String[1]);

	//
	ShowRecSerStrings(0x82, 0x1006, str);
	usleep(20000);

	//
	//gBMSInfo.BMSVolt.all = 6669;
	//gBMSInfo.BMSCur.all = 20;
	//gBMSInfo.SOC		= 88;
	//gBMSInfo.BMSCur_dischge_max.all = 2000;
	//gBMSInfo.BMSCur_chager_max.all = 1000;
	//
	ShowRecSerValue(0x82, 0x2000, gBMSInfo.BMSVolt.all);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2001, gBMSInfo.BMSCur.all - 30000);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2002, gBMSInfo.SOC * 10);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2003, gBMSInfo.BMSCur_dischge_max.all - 30000);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2004, gBMSInfo.BMSCur_chager_max.all - 30000);
	usleep(20000);
}


void UpdateSerPCS(void)
{
	uchar			String[4];
	char			str[8] =
		{
		0
		};
	char			str1[8] =
		{
		0
		};

	//
	//MonitorAck_BEG[DCModeNum - 1].Err_AC.all = 0x0000;
	String[0]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable1.all >> 4) & 0xf;
	String[1]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable1.all) & 0xf;
	String[2]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable2.all >> 4) & 0xf;
	String[3]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable2.all) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x1008, str);
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_AVolt.all = 2200;
	//MonitorAck_BEG[0].Module_AC_ACur.all = 100;
	//MonitorAck_BEG[0].Module_AC_AllP.all = 2200000;
	//
	//ShowRecSerValue(0x82, 0x2005, (uint16) (MonitorAck_BEG[0].Module_AC_AVolt.all));
	ShowRecSerValue(0x82, 0x2005, 
		(uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_AVolt.all / 3 + MonitorAck_BEG[DCModeNum - 1].Module_AC_BVolt.all / 3 + MonitorAck_BEG[DCModeNum - 1].Module_AC_CVolt.all / 3));
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2006, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_CCur.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_BCur.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_ACur.all)));

	//ShowRecSerValue(0x82, 0x2006, 
	//	(uint16) (gAMT_Info[6].PMAPhaseCur.all + gAMT_Info[6].PMBPhaseCur.all + gAMT_Info[6].PMCPhaseCur.all));
	usleep(20000);

	//
	//PM2_Power			= gAMT_Info[6].PMActivePower.all;
	PM2_Power			=
		 ((MonitorAck_BEG[DCModeNum - 1].Module_AC_CPower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_BPower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_APower.all) / 1000);

	//(MonitorAck_BEG[0].Module_AC_CPower.all + MonitorAck_BEG[0].Module_AC_BPower.all + MonitorAck_BEG[0].Module_AC_APower.all + MonitorAck_BEG[1].Module_AC_CPower.all + MonitorAck_BEG[1].Module_AC_BPower.all + MonitorAck_BEG[1].Module_AC_APower.all) / 1000;
	//
	ShowRecSerValue(0x82, 0x2007, 
		(uint16) (PM2_Power));
	usleep(20000);

	//
	//MonitorAck_BEG[DCModeNum - 1].Err.all = 0x0000;
	String[0]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable0.all >> 4) & 0xf;
	String[1]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable0.all) & 0xf;
	String[2]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable2.all >> 4) & 0xf;
	String[3]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable2.all) & 0xf;


	str1[0] 			= HexToChar(String[0]);
	str1[1] 			= HexToChar(String[1]);
	str1[2] 			= HexToChar(String[2]);
	str1[3] 			= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x100A, str1);
	usleep(20000);

}


void UpdateSerInfo(void)
{
	uchar			String[4];
	char			str[8] =
		{
		0
		};
	char			str1[8] =
		{
		0
		};
	char			str2[8] =
		{
		0
		};

	//
	uint32			sys_info = 0x0;
	uint32			config_info = 0x2FFA;
	uint32			real_info = 0x0;

	//
	//real_info			= (gBMSInfo.System_error & 0x00FF) + (gBMSInfo.System_error2 & 0x00FF) << 8;
	real_info			=
		 (SmartEnerg_io.IN_1 + SmartEnerg_io.IN_2 * 2 + SmartEnerg_io.IN_3 * 4 + SmartEnerg_io.IN_4 * 8);

	//sys_info			= gBMSInfo.Protection.all;
	sys_info			=
		 (gSmartEnerg_PGsys.SmartEnergy_WLchager << 12) + (gSmartEnerg_PGsys.SmartEnergy_DEGchager << 8) + (gSmartEnerg_sys.SmartEnergy_essrun << 4) +gSmartEnerg_sys.SmartEnergy_gridrun;

	//
	config_info 		= (gBMSInfo.BMS_Sta << 8) + (SmartEnerg_save << 4) +SmartEnerg_io.IN_3; //

	//
	String[0]			= (SmartEnerg_sta >> 4) & 0xf;
	String[1]			= (SmartEnerg_sta) & 0xf;
	String[2]			= (gBMSInfo.BMS_Sta >> 4 & 0xf);
	String[3]			= gBMSInfo.BMS_Sta & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x1000, str);
	usleep(20000);

	//
	String[0]			= (config_info >> 12) & 0xf;
	String[1]			= (config_info >> 8) & 0xf;
	String[2]			= (config_info >> 4 & 0xf);
	String[3]			= config_info & 0xf;

	str1[0] 			= HexToChar(String[0]);
	str1[1] 			= HexToChar(String[1]);
	str1[2] 			= HexToChar(String[2]);
	str1[3] 			= HexToChar(String[3]);

	//
	//ShowRecSerStrings(0x82, 0x1002, str1);
	//usleep(20000);
	//
	String[0]			= (real_info >> 12) & 0xf;
	String[1]			= (real_info >> 8) & 0xf;
	String[2]			= (real_info >> 4 & 0xf);
	String[3]			= real_info & 0xf;
	str2[0] 			= HexToChar(String[0]);
	str2[1] 			= HexToChar(String[1]);
	str2[2] 			= HexToChar(String[2]);
	str2[3] 			= HexToChar(String[3]);

	//
	//ShowRecSerStrings(0x82, 0x1004, str2);
	//usleep(20000);
}


void UpdateSerRollerr(void)
{ //¹ö¶¯ÏÔÊ¾ÐÅÏ¢£»

	ShowRecSerStrings_ERR(27, 0x82, 0x8003, "Energy Storage System!!");
	usleep(20000);
}


/*!
 *	\brief	¸üÐÂÏà¹ØÊý¾ÝÄÚÈÝ
 *	\param Éú²úÈÕÆÚ
 */
void UpdateDateRec0(void)
{
	uchar			iFor;
	uint16			addr = 0;
	uint32			date = 20190101;

	for (iFor = 0; iFor < 28; iFor = iFor + 2)
		{
		addr				= iFor + 0x0020;
		ShowRecDateAndProductValue(0x82, addr, date++);
		usleep(20000);
		}
}


/*!
 *	\brief	¸üÐÂÏà¹ØÊý¾ÝÄÚÈÝ
 *	\param 
 */
void UpdateSetCon_AA(uchar flag) //0:´ý»ú »ÒÉ« 1£ºÔËÐÐ À¶É« 2£º¸æ¾¯ ºìÉ« ÆäËû£º»ÒÉ«
{
	//
	if (flag == 0)
		{
		ShowRecSerValue(0x82, 0xB020, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9406, 61);		//ÉÏ±ê¸üÐÂ61Í¼Æ¬
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0xB020, 1);
			usleep(20000);
			}
		else 
			{
			if (flag == 2)
				{
				//
				ShowRecSerValue(0x82, 0x9406, 62);	//ÉÏ±ê¸üÐÂ61Í¼Æ¬
				usleep(20000);

				//
				ShowRecSerValue(0x82, 0xB020, 1);
				usleep(20000);
				}
			else 
				{
				ShowRecSerValue(0x82, 0xB020, 0);
				usleep(20000);
				}
			}
		}

}


//
void UpdateSetCon_WL(uchar flag) //0:´ý»ú »ÒÉ« 1£ºÔËÐÐ À¶É« 2£º¸æ¾¯ ºìÉ« ÆäËû£º»ÒÉ«
{
	//
	if (flag == 0)
		{
		ShowRecSerValue(0x82, 0xB120, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9B06, 61);		//ÉÏ±ê¸üÐÂ61Í¼Æ¬
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0xB120, 1);
			usleep(20000);
			}
		else 
			{
			if (flag == 2)
				{
				//
				ShowRecSerValue(0x82, 0x9B06, 62);	//ÉÏ±ê¸üÐÂ61Í¼Æ¬
				usleep(20000);

				//
				ShowRecSerValue(0x82, 0xB120, 1);
				usleep(20000);
				}
			else 
				{
				ShowRecSerValue(0x82, 0xB120, 0);
				usleep(20000);
				}
			}
		}

}


//
void UpdateSetLoad_AA(uchar flag)
{
	//
	if (flag == 0)
		{
		ShowRecSerValue(0x82, 0xB010, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9506, 71);		//ÉÏ±ê¸üÐÂ71Í¼Æ¬
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0xB010, 1);
			usleep(20000);
			}
		else 
			{
			if (flag == 2)
				{
				//
				ShowRecSerValue(0x82, 0x9506, 72);	//ÉÏ±ê¸üÐÂ71Í¼Æ¬
				usleep(20000);

				//
				ShowRecSerValue(0x82, 0xB010, 1);
				usleep(20000);
				}
			else 
				{
				ShowRecSerValue(0x82, 0xB010, 0);
				usleep(20000);
				}
			}
		}


}


void UpdateSetGrid_AA(uchar flag)
{
	if (flag == 0)
		{
		ShowRecSerValue(0x82, 0xB000, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9606, 81);		//ÉÏ±ê¸üÐÂ81Í¼Æ¬
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0xB000, 1);
			usleep(20000);
			}
		else 
			{
			if (flag == 2)
				{
				//
				ShowRecSerValue(0x82, 0x9606, 82);	//ÉÏ±ê¸üÐÂ81Í¼Æ¬
				usleep(20000);

				//
				ShowRecSerValue(0x82, 0xB000, 1);
				usleep(20000);
				}
			else 
				{
				ShowRecSerValue(0x82, 0xB000, 0);
				usleep(20000);
				}
			}
		}


}


void UpdateSetEss_AA(uchar flag, uchar gsoc, uchar dischge) ////0:´ý»ú »ÒÉ« 1£ºÔËÐÐ À¶É« 2£º¸æ¾¯ ºìÉ« ÆäËû£º»ÒÉ« //SOC 25%Ò»¸öµµ//dischge ³ä·Åµç
{
	if (flag == 0)
		{
		//
		if (gsoc <= 25)
			{
			ShowRecSerValue(0x82, 0x9006, 41);		//
			usleep(20000);
			ShowRecSerValue(0x82, 0x9007, 40);		//
			usleep(20000);
			ShowRecSerValue(0x82, 0x9008, 41);		//
			}
		else 
			{
			if ((gsoc > 25) && (gsoc <= 50))
				{
				ShowRecSerValue(0x82, 0x9006, 42);	//
				usleep(20000);
				ShowRecSerValue(0x82, 0x9007, 40);	//
				usleep(20000);
				ShowRecSerValue(0x82, 0x9008, 42);
				}
			else 
				{
				if ((gsoc > 50) && (gsoc <= 75))
					{
					ShowRecSerValue(0x82, 0x9006, 43); //
					usleep(20000);
					ShowRecSerValue(0x82, 0x9007, 40); //
					usleep(20000);
					ShowRecSerValue(0x82, 0x9008, 43);
					}
				else 
					{
					if ((gsoc > 75) && (gsoc <= 100))
						{
						ShowRecSerValue(0x82, 0x9006, 44); //
						usleep(20000);
						ShowRecSerValue(0x82, 0x9007, 40); //
						usleep(20000);
						ShowRecSerValue(0x82, 0x9008, 44);
						}
					}
				}
			}

		usleep(20000);

		ShowRecSerValue(0x82, 0x3004, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			ShowRecSerValue(0x82, 0x9006, 45);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9007, 45);		//
			usleep(20000);

			//
			if (gsoc <= 25)
				ShowRecSerValue(0x82, 0x9008, 45); //
			else 
				{
				if ((gsoc > 25) && (gsoc <= 50))
					ShowRecSerValue(0x82, 0x9008, 46);
				else 
					{
					if ((gsoc > 50) && (gsoc <= 75))
						ShowRecSerValue(0x82, 0x9008, 47);
					else 
						{
						if ((gsoc > 75) && (gsoc <= 100))
							ShowRecSerValue(0x82, 0x9008, 48);
						}
					}
				}

			usleep(20000);

			ShowRecSerValue(0x82, 0x3004, 1);
			usleep(20000);

			}
		else 
			{
			if (flag == 2)
				{
				ShowRecSerValue(0x82, 0x9006, 53);	//
				usleep(20000);

				//
				ShowRecSerValue(0x82, 0x9007, 53);	//
				usleep(20000);

				//
				if (gsoc <= 25)
					ShowRecSerValue(0x82, 0x9008, 53); //
				else 
					{
					if ((gsoc > 25) && (gsoc <= 50))
						ShowRecSerValue(0x82, 0x9008, 54);
					else 
						{
						if ((gsoc > 50) && (gsoc <= 75))
							ShowRecSerValue(0x82, 0x9008, 55);
						else 
							{
							if ((gsoc > 75) && (gsoc <= 100))
								ShowRecSerValue(0x82, 0x9008, 56);
							}
						}
					}

				usleep(20000);

				ShowRecSerValue(0x82, 0x3004, 1);
				usleep(20000);
				}
			else 
				{
				//
				if (gsoc <= 25)
					{
					ShowRecSerValue(0x82, 0x9006, 41); //
					usleep(20000);
					ShowRecSerValue(0x82, 0x9007, 40); //
					usleep(20000);
					ShowRecSerValue(0x82, 0x9008, 41); //
					}
				else 
					{
					if ((gsoc > 25) && (gsoc <= 50))
						{
						ShowRecSerValue(0x82, 0x9006, 42); //
						usleep(20000);
						ShowRecSerValue(0x82, 0x9007, 40); //
						usleep(20000);
						ShowRecSerValue(0x82, 0x9008, 42);
						}
					else 
						{
						if ((gsoc > 50) && (gsoc <= 75))
							{
							ShowRecSerValue(0x82, 0x9006, 43); //
							usleep(20000);
							ShowRecSerValue(0x82, 0x9007, 40); //
							usleep(20000);
							ShowRecSerValue(0x82, 0x9008, 43);
							}
						else 
							{
							if ((gsoc > 75) && (gsoc <= 100))
								{
								ShowRecSerValue(0x82, 0x9006, 44); //
								usleep(20000);
								ShowRecSerValue(0x82, 0x9007, 40); //
								usleep(20000);
								ShowRecSerValue(0x82, 0x9008, 44);
								}
							}
						}
					}

				usleep(20000);

				ShowRecSerValue(0x82, 0x3004, 0);
				usleep(20000);


				}
			}
		}

}


void UpdateSetcur_2408_ESS_PCS(uchar flag)
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x9106, 82);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9107, 80);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9108, 82);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x3016, 0xFF00);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9106, 82);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9107, 80);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9108, 82);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x3016, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}

}


void UpdateSetcur1_L(uchar flag) ////0:¾²Ö¹ 1£ºÔËÐÐ,<----
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x9106, 18);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9107, 15);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9108, 18);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x3016, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9106, 18);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9107, 15);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9108, 18);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x3016, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}

}


void UpdateSetcur1_R(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x9106, 14);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9107, 10);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9108, 14);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x3016, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9106, 14);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9107, 10);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9108, 14);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x3016, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}

}


void UpdateSetcur_PCS_LD(uchar flag)
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x9206, 82);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9207, 80);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9208, 82);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x3018, 0xFF00);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9206, 82);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9207, 80);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9208, 82);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x3018, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}

}


void UpdateSetcur2_L(uchar flag)
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x9206, 28);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9207, 25);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9208, 28);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x3018, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9206, 28);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9207, 25);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9208, 28);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x3018, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}

}


void UpdateSetcur2_R(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x9206, 24);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9207, 20);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9208, 24);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x3018, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9206, 24);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9207, 20);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9208, 24);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x3018, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}

}


void UpdateSetcur3_L(uchar flag)
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x9306, 34);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9307, 30);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9308, 34);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x301A, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9306, 34);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9307, 30);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9308, 34);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x301A, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}
}


void UpdateSetcur3_R(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x9306, 38);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9307, 35);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9308, 38);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x301A, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9306, 38);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9307, 35);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9308, 38);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x301A, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}
}


void UpdateSetcur_AC_CCS_MD(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x9306, 51);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9307, 40);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9308, 51);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x301A, 0xFF00);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9306, 51);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9307, 40);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9308, 51);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x301A, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}
}


void UpdateSetcur_AC_MD_ESS(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x8B06, 91);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x8B07, 90);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x8B08, 91);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x3030, 0xFF00);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x8B06, 91);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x8B07, 90);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x8B08, 91);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x3030, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}
}


void UpdateSetcur_DC_CCS_ESS(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x8C06, 71);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x8C07, 60);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x8C08, 71);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x3034, 0xFF00);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x8C06, 71);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x8C07, 60);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x8C08, 71);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x3034, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}
}


void UpdateSetcur_ESS_CDZ(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x8D06, 102); 		//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x8D07, 100); 		//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x8D08, 102); 		//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x3036, 0xFF00);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x8D06, 102); 	//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x8D07, 100); 	//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x8D08, 102); 	//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x3036, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}
}


void UpdateSetcur_CDZ_EV(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x8E06, 102); 		//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x8E07, 100); 		//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x8E08, 102); 		//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x3038, 0xFF00);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x8E06, 102); 	//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x8E07, 100); 	//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x8E08, 102); 	//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x3038, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}
}


///////////////////////////
void UpdateSetcur4_L(uchar flag)
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x9706, 28);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9707, 25);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9708, 28);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x301C, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9706, 28);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9707, 25);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9708, 28);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x301C, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}
}


void UpdateSetcur5_R(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x9806, 14);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9807, 10);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9808, 14);			//

		ShowRecSerValue(0x82, 0x301E, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9806, 14);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9807, 10);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9808, 14);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x301E, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}
}


void UpdateSetcur6_R(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x9906, 24);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9907, 20);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9908, 24);			//

		ShowRecSerValue(0x82, 0x3020, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9906, 24);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9907, 20);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9908, 24);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x3020, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}
}


void UpdateSetcur7_L(uchar flag)
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x9A06, 34);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9A07, 30);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x9A08, 34);			//
		usleep(20000);

		//
		ShowRecSerValue(0x82, 0x3022, 0);
		usleep(20000);
		}
	else 
		{
		if (flag == 1)
			{
			//
			ShowRecSerValue(0x82, 0x9A06, 34);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9A07, 30);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x9A08, 34);		//
			usleep(20000);

			//
			ShowRecSerValue(0x82, 0x3022, 1);
			usleep(20000);

			}
		else 
			{
			//
			}
		}
}


///////////////////////////////////////////////////////////////
void UpdateUI_Page63(void) //×Ô¼ìÒ³Ãæ
{
}


void UpdateUI_Page64(void) //Ö÷Ò³Ãæ
{
	UpdateEsc();
	usleep(20000);

	//UpdateSerPCS();
	//usleep(20000);
	UpdateSerInfo();
	usleep(20000);
	UpdateUI_B2408_Page64();

}


uint16			Time = 0;


void UpdateUI_Page70(void) //PCSÐÅÏ¢

{
	uchar			String[4];

	//
	char			str[8] =
		{
		0
		};
	char			str1[8] =
		{
		0
		};
	char			str2[8] =
		{
		0
		};
	char			str3[8] =
		{
		0
		};
	char			str4[8] =
		{
		0
		};
	char			str5[8] =
		{
		0
		};
	char			str6[8] =
		{
		0
		};

	///AC_info
	//MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable0.all = 0x0000;
	//
	String[0]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable0.all >> 4) & 0xf;
	String[1]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable0.all) & 0xf;
	String[2]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable2.all >> 4) & 0xf;
	String[3]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable2.all) & 0xf;


	str1[0] 			= HexToChar(String[0]);
	str1[1] 			= HexToChar(String[1]);
	str1[2] 			= HexToChar(String[2]);
	str1[3] 			= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x100C, str1);
	usleep(20000);

	//
	//ShowRecSerValue(0x82, 0x2008, 
	//	(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_AllP.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_AllLP.all) / 100));
	ShowRecSerValue(0x82, 0x2008, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_CPower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_BPower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_APower.all) / 10000));

	//ShowRecSerValue(0x82, 0x2008, 
	//	(uint16) (gAMT_Info[6].PMActivePower.all));
	usleep(20000);

	//
	//290Ah=17400Am
	//
	if (gBMSInfo.BMSCur.all == 30000)
		{
		Time				= 999;
		}
	else 
		{
		if (gBMSInfo.BMSCur.all > 30000)
			Time = 174 * gBMSInfo.SOC / ((gBMSInfo.BMSCur.all / 10) - 3000);
		else 
			Time = 174 * gBMSInfo.SOC / (3000 - (gBMSInfo.BMSCur.all / 10));
		}

	//
	ShowRecSerValue(0x82, 0x2009, (uint16) (Time));

	//if (timer_tick_count_[7] > 60) //B2408
	//{
	//	Time++;
	//	timer_tick_count_[7] = 0;
	//}
	//if (Time >= 999)
	//	Time = 0;
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2009, (uint16) (Time));

	if (timer_tick_count_[7] > 60) //B2408
		{
		Time++;
		timer_tick_count_[7] = 0;
		}

	//if (Time >= 999)
	//	Time = 0;
	usleep(20000);

	//
	//MonitorAck_BEG[0].Temp = 470;
	//MonitorAck_BEG[1].Temp = 570;
	//MonitorAck_BEG[2].Temp = 670;
	//
	ShowRecSerValue(0x82, 0x200A, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Temp) / 10));
	usleep(20000);

	////////////////////////////////////////////A
	//MonitorAck_BEG[0].Module_AC_AVolt.all = gAMT_Info[6].PMAPhaseVolt.all;
	ShowRecSerValue(0x82, 0x200B, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_AVolt.all) / 10);
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_ABVolt.all = gAMT_Info[6].PMAPhaseVolt.all;
	ShowRecSerValue(0x82, 0x200C, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_ABVolt.all) / 10);
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_ACur.all = gAMT_Info[6].PMAPhaseCur.all;
	//ShowRecSerValue(0x82, 0x200D, 
	//	(uint16) ((MonitorAck_BEG[0].Module_AC_ACur.all + MonitorAck_BEG[1].Module_AC_ACur.all) / 10));
	ShowRecSerValue(0x82, 0x200D, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_ACur.all) / 10));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_APower.all = gAMT_Info[6].PMActivePower.all / 3;
	//ShowRecSerValue(0x82, 0x200E, 
	//	(uint16) ((MonitorAck_BEG[0].Module_AC_APower.all + MonitorAck_BEG[1].Module_AC_APower.all) / 10000));
	ShowRecSerValue(0x82, 0x200E, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_APower.all) / 10000));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_LAPower.all = gAMT_Info[6].PMIddlePower.all / 3;
	//ShowRecSerValue(0x82, 0x200F, 
	//	(uint16) ((MonitorAck_BEG[0].Module_AC_LAPower.all + MonitorAck_BEG[1].Module_AC_LAPower.all) / 10000));
	ShowRecSerValue(0x82, 0x200F, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_LAPower.all) / 10000));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_HZ.all = gAMT_Info[6].PMFq.all;
	ShowRecSerValue(0x82, 0x2010, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_HZ.all / 10));
	usleep(20000);

	////////////////////////////////////////////B
	//MonitorAck_BEG[0].Module_AC_BVolt.all = gAMT_Info[6].PMBPhaseVolt.all;
	ShowRecSerValue(0x82, 0x2011, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_BVolt.all / 10));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_BCVolt.all = gAMT_Info[6].PMBPhaseVolt.all;
	ShowRecSerValue(0x82, 0x2012, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_BCVolt.all / 10));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_BCur.all = gAMT_Info[6].PMBPhaseCur.all;
	//ShowRecSerValue(0x82, 0x2013, 
	//	(uint16) ((MonitorAck_BEG[0].Module_AC_BCur.all + MonitorAck_BEG[1].Module_AC_BCur.all) / 10));
	ShowRecSerValue(0x82, 0x2013, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_BCur.all) / 10));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_BPower.all = gAMT_Info[6].PMActivePower.all / 3;
	//ShowRecSerValue(0x82, 0x2014, 
	//	(uint16) ((MonitorAck_BEG[0].Module_AC_BPower.all + MonitorAck_BEG[1].Module_AC_BPower.all) / 10000));
	ShowRecSerValue(0x82, 0x2014, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_BPower.all) / 10000));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_LBPower.all = gAMT_Info[6].PMIddlePower.all / 3;
	//ShowRecSerValue(0x82, 0x2015, 
	//	(uint16) ((MonitorAck_BEG[0].Module_AC_LBPower.all + MonitorAck_BEG[1].Module_AC_LBPower.all) / 10000));
	ShowRecSerValue(0x82, 0x2015, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_LBPower.all) / 10000));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_HZ.all = gAMT_Info[6].PMFq.all;
	ShowRecSerValue(0x82, 0x2016, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_HZ.all / 10));
	usleep(20000);

	////////////////////////////////////////////C
	//MonitorAck_BEG[0].Module_AC_CVolt.all = gAMT_Info[6].PMCPhaseVolt.all;
	ShowRecSerValue(0x82, 0x2017, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_CVolt.all / 10));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_CVolt.all = gAMT_Info[6].PMCPhaseVolt.all;
	ShowRecSerValue(0x82, 0x2018, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_CAVolt.all / 10));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_CCur.all = gAMT_Info[6].PMCPhaseCur.all;
	//ShowRecSerValue(0x82, 0x2019, 
	//	(uint16) ((MonitorAck_BEG[0].Module_AC_CCur.all + MonitorAck_BEG[1].Module_AC_CCur.all) / 10));
	ShowRecSerValue(0x82, 0x2019, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_CCur.all) / 10));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_CPower.all = gAMT_Info[6].PMActivePower.all / 3;
	//ShowRecSerValue(0x82, 0x201A, 
	//	(uint16) ((MonitorAck_BEG[0].Module_AC_CPower.all + MonitorAck_BEG[1].Module_AC_CPower.all) / 10000));
	ShowRecSerValue(0x82, 0x201A, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_CPower.all) / 10000));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_LCPower.all = gAMT_Info[6].PMIddlePower.all / 3;
	//ShowRecSerValue(0x82, 0x201B, 
	//	(uint16) ((MonitorAck_BEG[0].Module_AC_LCPower.all + MonitorAck_BEG[1].Module_AC_LCPower.all) / 10000));
	ShowRecSerValue(0x82, 0x201B, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_LCPower.all) / 10000));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_HZ.all = gAMT_Info[6].PMFq.all;
	ShowRecSerValue(0x82, 0x201C, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_HZ.all / 10));
	usleep(20000);

	////////////////////////////////////////////ALL
	//MonitorAck_BEG[0].Module_AC_CVolt.all = gAMT_Info[6].PMCPhaseVolt.all;
	ShowRecSerValue(0x82, 0x201D, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_CVolt.all / 10));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_CAVolt.all =
	//	 gAMT_Info[0].PMAPhaseVolt.all / 3 + gAMT_Info[0].PMBPhaseVolt.all / 3 + gAMT_Info[0].PMCPhaseVolt.all / 3;
	ShowRecSerValue(0x82, 0x201E, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_CAVolt.all / 10));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_CCur.all = 20000;
	ShowRecSerValue(0x82, 0x201F, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_CCur.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_BCur.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_ACur.all) / 10));

	//ShowRecSerValue(0x82, 0x201F, 
	//	(uint16) (gAMT_Info[0].PMAPhaseCur.all + gAMT_Info[0].PMBPhaseCur.all + gAMT_Info[0].PMCPhaseCur.all));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_CPower.all = 440000;
	ShowRecSerValue(0x82, 0x2020, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_CPower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_BPower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_APower.all) / 10000));

	//ShowRecSerValue(0x82, 0x2020, 
	//	(uint16) (gAMT_Info[6].PMActivePower.all));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_LCPower.all = 45000;
	ShowRecSerValue(0x82, 0x2021, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_LCPower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_LBPower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_LAPower.all) / 10000));

	//ShowRecSerValue(0x82, 0x2021, 
	//	(uint16) (gAMT_Info[6].PMIddlePower.all));
	usleep(20000);

	//
	//MonitorAck_BEG[0].Module_AC_HZ.all = gAMT_Info[6].PMFq.all;
	ShowRecSerValue(0x82, 0x2022, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_HZ.all / 10));
	usleep(20000);


	//////////////////////////////////////////
	//HV
	//MonitorAck_BEG[0].ModuleVoltOut.all = gBMSInfo.BMSVolt.all;
	ShowRecSerValue(0x82, 0x2023, (uint16) (gBMSInfo.BMSVolt.all));
	usleep(20000);

	//
	//MonitorAck_BEG[0].ModuleCurOut.all = gBMSInfo.BMSCur.all - 30000;
	//ShowRecSerValue(0x82, 0x2024, 
	//	(uint16) ((MonitorAck_BEG[0].ModuleCurOut.all + MonitorAck_BEG[1].ModuleCurOut.all)));
	ShowRecSerValue(0x82, 0x2024, 
		(uint16) ((gBMSInfo.BMSCur.all - 30000)));
	usleep(20000);

	//MonitorAck_BEG[0].ModuleCurOut.all = 50000;
	//ShowRecSerValue(0x82, 0x2025, 
	//	(uint16) ((MonitorAck_BEG[0].ModuleVoltOut.all / 100) * ((MonitorAck_BEG[0].ModuleCurOut.all + MonitorAck_BEG[1].ModuleCurOut.all) / 100)));
	if (gBMSInfo.BMSCur.all >= 30000)
		{
		ShowRecSerValue(0x82, 0x2025, 
			(uint16) ((gBMSInfo.BMSVolt.all / 10) * (((gBMSInfo.BMSCur.all - 30000)) / 10)) / 100);
		}
	else 
		{
		ShowRecSerValue(0x82, 0x2025, 
			(uint16) ((gBMSInfo.BMSVolt.all / 10) * (((30000 - gBMSInfo.BMSCur.all)) / 10)) / 100);

		}


	usleep(20000);

	//LV
	//MonitorAck_BEG[0].ModuleVoltOut.all = 650000;
	ShowRecSerValue(0x82, 0x2026, (uint16) (gBMSInfo.BMSVolt.all));
	usleep(20000);

	//
	//MonitorAck_BEG[0].ModuleCurOut.all = 50000;
	//ShowRecSerValue(0x82, 0x2027, 
	//	(uint16) ((MonitorAck_BEG[0].ModuleCurOut.all + MonitorAck_BEG[1].ModuleCurOut.all)));
	ShowRecSerValue(0x82, 0x2027, 
		(uint16) (((gBMSInfo.BMSCur.all - 30000))));
	usleep(20000);

	//MonitorAck_BEG[0].ModuleCurOut.all = 50000;
	//ShowRecSerValue(0x82, 0x2028, 
	//	(uint16) ((MonitorAck_BEG[0].ModuleVoltOut.all / 100) * ((MonitorAck_BEG[0].ModuleCurOut.all + MonitorAck_BEG[1].ModuleCurOut.all) / 100)));
	if (gBMSInfo.BMSCur.all >= 30000)
		{
		ShowRecSerValue(0x82, 0x2028, 
			(uint16) ((gBMSInfo.BMSVolt.all / 10) * (((gBMSInfo.BMSCur.all - 30000)) / 10)) / 100);
		}
	else 
		{
		ShowRecSerValue(0x82, 0x2028, 
			(uint16) ((gBMSInfo.BMSVolt.all / 10) * (((30000 - gBMSInfo.BMSCur.all)) / 10)) / 100);

		}

	usleep(20000);

	//////////////////////////////////////////
	String[0]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable0.all >> 4) & 0xf;
	String[1]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable0.all) & 0xf;
	String[2]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable0.all >> 4) & 0xf;
	String[3]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable0.all) & 0xf;
	str1[0] 			= HexToChar(String[0]);
	str1[1] 			= HexToChar(String[1]);
	str1[2] 			= HexToChar(String[2]);
	str1[3] 			= HexToChar(String[3]);
	String[0]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable1.all >> 4) & 0xf;
	String[1]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable1.all) & 0xf;
	String[2]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable2.all >> 4) & 0xf;
	String[3]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable2.all) & 0xf;
	str2[0] 			= HexToChar(String[0]);
	str2[1] 			= HexToChar(String[1]);
	str2[2] 			= HexToChar(String[2]);
	str2[3] 			= HexToChar(String[3]);
	String[0]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable2.all >> 4) & 0xf;
	String[1]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable2.all) & 0xf;
	String[2]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable2.all >> 4) & 0xf;
	String[3]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable2.all) & 0xf;
	str3[0] 			= HexToChar(String[0]);
	str3[1] 			= HexToChar(String[1]);
	str3[2] 			= HexToChar(String[2]);
	str3[3] 			= HexToChar(String[3]);

	/*
	str1[0] 			= ' ';
	str1[1] 			= ' ';
	str1[2] 			= ' ';
	str1[3] 			= ' ';
	str2[0] 			= ' ';
	str2[1] 			= ' ';
	str2[2] 			= ' ';
	str2[3] 			= ' ';
	str3[0] 			= ' ';
	str3[1] 			= ' ';
	str3[2] 			= ' ';
	str3[3] 			= ' ';
	str4[0] 			= ' ';
	str4[1] 			= ' ';
	str4[2] 			= ' ';
	str4[3] 			= ' ';
	*/
	ShowRecSerStrings(0x82, 0x100E, str1);
	usleep(20000);
	ShowRecSerStrings(0x82, 0x1010, str2);
	usleep(20000);


	ShowRecSerStrings(0x82, 0x1012, str3);
	usleep(20000);
	ShowRecSerStrings(0x82, 0x1014, "FFFF");
	usleep(20000);

	/*
	ShowRecSerStrings(0x82, 0x1016, "FFFF");
	usleep(20000);
	ShowRecSerStrings(0x82, 0x1018, "FFFF");
	usleep(20000);
	ShowRecSerStrings(0x82, 0x100E, str1);
	usleep(20000);
	ShowRecSerStrings(0x82, 0x1010, str2);
	usleep(20000);
	ShowRecSerStrings(0x82, 0x1012, str3);
	usleep(20000);
	ShowRecSerStrings(0x82, 0x1014, "FFFF");
	usleep(20000);
	ShowRecSerStrings(0x82, 0x1016, "FFFF");
	usleep(20000);
	ShowRecSerStrings(0x82, 0x1018, "FFFF");
	usleep(20000);
	*/
	//////////////////////////////////////////
	ShowRecSerValue(0x82, 0x2029, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Temp) / 10));
	usleep(20000);

	//ACDCµç±í£¨ÐéÄâ£©
	//
	ShowRecSerValue2(0x82, 0x2580, (uint32) (Chagre_once_KWH)); //µ±Ç°
	usleep(20000);
	ShowRecSerValue2(0x82, 0x2590, (uint32) (Chagre_all_KWH)); //×Ü¹²
	usleep(20000);

	ShowRecSerValue2(0x82, 0x25A0, (uint32) (DisChagre_once_KWH)); //µ±Ç°
	usleep(20000);
	ShowRecSerValue2(0x82, 0x25B0, (uint32) (DisChagre_all_KWH)); //×Ü¹²
	usleep(20000);

	//
	String[0]			= (OBD_ACDC_Sta1 >> 4) & 0xf;
	String[1]			= (OBD_ACDC_Sta1) & 0xf;
	String[2]			= (OBD_ACDC_Sta2 >> 4) & 0xf;
	String[3]			= (OBD_ACDC_Sta2) & 0xf;
	str1[0] 			= HexToChar(String[0]);
	str1[1] 			= HexToChar(String[1]);
	str1[2] 			= HexToChar(String[2]);
	str1[3] 			= HexToChar(String[3]);
	ShowRecSerStrings(0x82, 0x1510, str1);			//
	usleep(20000);
	String[0]			= (OBD_ACDC_Sta3 >> 4) & 0xf;
	String[1]			= (OBD_ACDC_Sta3) & 0xf;
	String[2]			= (0xFF >> 4) & 0xf;
	String[3]			= (0xFF) & 0xf;
	str1[0] 			= HexToChar(String[0]);
	str1[1] 			= HexToChar(String[1]);
	str1[2] 			= HexToChar(String[2]);
	str1[3] 			= HexToChar(String[3]);
	ShowRecSerStrings(0x82, 0x1512, str1);			//
	usleep(20000);
	String[0]			= (0xFF >> 4) & 0xf;
	String[1]			= (0xFF) & 0xf;
	String[2]			= (0xFF >> 4) & 0xf;
	String[3]			= (0xFF) & 0xf;
	str1[0] 			= HexToChar(String[0]);
	str1[1] 			= HexToChar(String[1]);
	str1[2] 			= HexToChar(String[2]);
	str1[3] 			= HexToChar(String[3]);
	ShowRecSerStrings(0x82, 0x1514, str1);			//
	usleep(20000);

}


void UpdateUI_B2408_Page64(void) //B2408-pcs
{
	uchar			String[4];

	//
	char			str[4] =
		{
		0
		};

	//
	ShowRecSerValue(0x82, 0x2008, 
		(uint16) ((MonitorAck_BEG[DCModeNum - 1].Module_AC_CPower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_BPower.all + MonitorAck_BEG[DCModeNum - 1].Module_AC_APower.all) / 10000));

	//ShowRecSerValue(0x82, 0x2008, 
	//	(uint16) (gAMT_Info[6].PMActivePower.all));
	usleep(20000);

	ShowRecSerValue(0x82, 0x200B, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_AVolt.all) / 10);
	usleep(20000);
	ShowRecSerValue(0x82, 0x2011, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_BVolt.all / 10));
	usleep(20000);
	ShowRecSerValue(0x82, 0x2017, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_CVolt.all / 10));
	usleep(20000);

	ShowRecSerValue(0x82, 0x2010, (uint16) (MonitorAck_BEG[DCModeNum - 1].Module_AC_HZ.all / 10));
	usleep(20000);

	/////////////////////////
	ShowRecSerValue(0x82, 0x202C, 
		(uint16) (gAMT_Info[0].PMAPhaseVolt.all / 3 + gAMT_Info[0].PMBPhaseVolt.all / 3 + gAMT_Info[0].PMCPhaseVolt.all / 3));
	usleep(20000);
	ShowRecSerValue(0x82, 0x202F, (uint16) (gAMT_Info[0].PMFq.all));
	usleep(20000);
	ShowRecSerValue(0x82, 0x202A, (uint16) (gAMT_Info[0].PMActivePower.all));
	usleep(20000);

	//
	if (gBMSInfo.BMSCur.all >= 30000)
		{
		ShowRecSerValue(0x82, 0x2025, 
			(uint16) ((gBMSInfo.BMSVolt.all / 10) * (((gBMSInfo.BMSCur.all - 30000)) / 10)) / 100);
		}
	else 
		{
		ShowRecSerValue(0x82, 0x2025, 
			(uint16) ((gBMSInfo.BMSVolt.all / 10) * (((30000 - gBMSInfo.BMSCur.all)) / 10)) / 100);

		}


	usleep(20000);

	//
	if (CF_Secc_DCAC_ChgMode == 0)
		{
		ShowRecSerStrings(0x82, 0x1504, "DISC");
		usleep(20000);
		ShowRecSerStrings(0x82, 0x1506, "DISC");
		usleep(20000);
		}
	else 
		{
		if (CF_Secc_DCAC_ChgMode == 1)
			{
			ShowRecSerStrings(0x82, 0x1504, "DISC");
			usleep(20000);
			ShowRecSerStrings(0x82, 0x1506, "LINK");
			usleep(20000);
			}
		else 
			{
			ShowRecSerStrings(0x82, 0x1504, "LINK");
			usleep(20000);
			ShowRecSerStrings(0x82, 0x1506, "DISC");
			usleep(20000);
			}
		}

	String[0]			= (BMS_fual_max >> 4) & 0xf;
	String[1]			= (BMS_fual_max) & 0xf;
	String[2]			= (gBMSInfo.Error >> 4) & 0xf;
	String[3]			= (gBMSInfo.Error) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);


	//
	ShowRecSerStrings(0x82, 0x102C, str);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2044, gBMSInfo.BMSCur_dischge_max.all - 30000);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2045, gBMSInfo.BMSCur_chager_max.all - 30000);
	usleep(20000);

}


void UpdateUI_Page71(void) //¸ºÔØÐÅÏ¢
{
	///////////////Load1	
	//
	//gAMT_Info[1].PMActivePower.all = 4200;
	ShowRecSerValue(0x82, 0x202A, (uint16) (gAMT_Info[0].PMActivePower.all));
	usleep(20000);

	//
	//gAMT_Info[1].PMEnergy.all = 8000;
	ShowRecSerValue2(0x82, 0x2500, (uint32) (gAMT_Info[0].PMEnergy.all));
	usleep(20000);

	//
	//gAMT_Info[1].PM_harmonics_V.all = 10;
	ShowRecSerValue(0x82, 0x202C, 
		(uint16) (gAMT_Info[0].PMAPhaseVolt.all / 3 + gAMT_Info[0].PMBPhaseVolt.all / 3 + gAMT_Info[0].PMCPhaseVolt.all / 3));
	usleep(20000);

	//
	//gAMT_Info[1].PM_harmonics_I.all = 9;
	ShowRecSerValue(0x82, 0x202D, 
		(uint16) (gAMT_Info[0].PMAPhaseCur.all + gAMT_Info[0].PMBPhaseCur.all + gAMT_Info[0].PMCPhaseCur.all));
	usleep(20000);

	//
	//gAMT_Info[1].PMPF.all = 10;
	ShowRecSerValue(0x82, 0x202B, (uint16) (gAMT_Info[0].PMPF.all));
	usleep(20000);

	//
	//gAMT_Info[1].PMFq.all = 500;
	ShowRecSerValue(0x82, 0x202F, (uint16) (gAMT_Info[0].PMFq.all));
	usleep(20000);

	//////////////////////////////////////////
	///////////////Load2	
	//
	//gAMT_Info[2].PMActivePower.all = 4200;
	ShowRecSerValue(0x82, 0x2030, (uint16) (gAMT_Info[1].PMActivePower.all));
	usleep(20000);

	//
	gAMT_Info[2].PMEnergy.all = gAMT_Info[1].PMEnergy.all;
	ShowRecSerValue2(0x82, 0x2510, (uint32) (gAMT_Info[1].PMEnergy.all));
	usleep(20000);

	//
	//gAMT_Info[2].PM_harmonics_V.all = 10;
	ShowRecSerValue(0x82, 0x2032, 
		(uint16) (gAMT_Info[1].PMAPhaseVolt.all / 3 + gAMT_Info[1].PMBPhaseVolt.all / 3 + gAMT_Info[1].PMCPhaseVolt.all / 3));
	usleep(20000);

	//
	//gAMT_Info[2].PM_harmonics_I.all = 9;
	ShowRecSerValue(0x82, 0x2033, 
		(uint16) (gAMT_Info[1].PMAPhaseCur.all + gAMT_Info[1].PMBPhaseCur.all + gAMT_Info[1].PMCPhaseCur.all));
	usleep(20000);

	//
	//gAMT_Info[2].PMPF.all = 10;
	ShowRecSerValue(0x82, 0x2031, (uint16) (gAMT_Info[1].PMPF.all));
	usleep(20000);

	//
	//gAMT_Info[2].PMFq.all = 510;
	ShowRecSerValue(0x82, 0x2035, (uint16) (gAMT_Info[1].PMFq.all));
	usleep(20000);

	//////////////////////////////////////////
	///////////////Load3	
	//
	//gAMT_Info[3].PMActivePower.all = 4200;
	ShowRecSerValue(0x82, 0x2036, (uint16) (gAMT_Info[4].PMActivePower.all));
	usleep(20000);

	//
	//gAMT_Info[3].PMEnergy.all = 8000;
	ShowRecSerValue2(0x82, 0x2520, (uint32) (gAMT_Info[4].PMEnergy.all));
	usleep(20000);

	//
	//gAMT_Info[3].PM_harmonics_V.all = 10;
	ShowRecSerValue(0x82, 0x2038, (uint16) (gAMT_Info[4].PMAPhaseVolt.all));
	usleep(20000);

	//
	//gAMT_Info[3].PM_harmonics_I.all = 9;
	ShowRecSerValue(0x82, 0x2039, (uint16) (gAMT_Info[4].PMAPhaseCur.all));
	usleep(20000);

	//
	//gAMT_Info[3].PMPF.all = 10;
	ShowRecSerValue(0x82, 0x2037, (uint16) (gAMT_Info[4].PMPF.all));
	usleep(20000);

	//
	//gAMT_Info[3].PMFq.all = 520;
	ShowRecSerValue(0x82, 0x203B, (uint16) (gAMT_Info[4].PMFq.all));
	usleep(20000);

	//////////////////////////////////////////
}


void UpdateUI_Page113(void) //¸ºÔØÐÅÏ¢
{
	///////////////Load1	
	//
	//gAMT_Info[1].PMActivePower.all = 4200;
	ShowRecSerValue(0x82, 0x202A, (uint16) (gAMT_Info[2].PMActivePower.all));
	usleep(20000);

	//
	//gAMT_Info[1].PMEnergy.all = 8000;
	ShowRecSerValue2(0x82, 0x2530, (uint32) (gAMT_Info[2].PMEnergy.all));
	usleep(20000);

	//
	//gAMT_Info[1].PM_harmonics_V.all = 10;
	ShowRecSerValue(0x82, 0x202C, 
		(uint16) (gAMT_Info[2].PMAPhaseVolt.all / 3 + gAMT_Info[2].PMBPhaseVolt.all / 3 + gAMT_Info[2].PMCPhaseVolt.all / 3));
	usleep(20000);

	//
	//gAMT_Info[1].PM_harmonics_I.all = 9;
	//
	ShowRecSerValue(0x82, 0x202D, 
		(uint16) (gAMT_Info[2].PMAPhaseCur.all + gAMT_Info[2].PMBPhaseCur.all + gAMT_Info[2].PMCPhaseCur.all));
	usleep(20000);

	//
	//gAMT_Info[1].PMPF.all = 10;
	ShowRecSerValue(0x82, 0x202B, (uint16) (gAMT_Info[2].PMPF.all));
	usleep(20000);

	//
	//gAMT_Info[1].PMFq.all = 500;
	ShowRecSerValue(0x82, 0x202F, (uint16) (gAMT_Info[2].PMFq.all));
	usleep(20000);

	//////////////////////////////////////////
	///////////////Load2	
	//
	//gAMT_Info[2].PMActivePower.all = 4200;
	ShowRecSerValue(0x82, 0x2030, (uint16) (gAMT_Info[3].PMActivePower.all));
	usleep(20000);

	//
	gAMT_Info[2].PMEnergy.all = gAMT_Info[1].PMEnergy.all;
	ShowRecSerValue2(0x82, 0x2540, (uint32) (gAMT_Info[3].PMEnergy.all));
	usleep(20000);

	//
	//gAMT_Info[2].PM_harmonics_V.all = 10;
	ShowRecSerValue(0x82, 0x2032, 
		(uint16) (gAMT_Info[3].PMAPhaseVolt.all / 3 + gAMT_Info[3].PMBPhaseVolt.all / 3 + gAMT_Info[3].PMCPhaseVolt.all / 3));
	usleep(20000);

	//
	//gAMT_Info[2].PM_harmonics_I.all = 9;
	ShowRecSerValue(0x82, 0x2033, 
		(uint16) (gAMT_Info[3].PMAPhaseCur.all + gAMT_Info[3].PMBPhaseCur.all + gAMT_Info[3].PMCPhaseCur.all));
	usleep(20000);

	//
	//gAMT_Info[2].PMPF.all = 10;
	ShowRecSerValue(0x82, 0x2031, (uint16) (gAMT_Info[3].PMPF.all));
	usleep(20000);

	//
	//gAMT_Info[2].PMFq.all = 510;
	ShowRecSerValue(0x82, 0x2035, (uint16) (gAMT_Info[3].PMFq.all));
	usleep(20000);

	//////////////////////////////////////////
	///////////////Load3	
	//
	//gAMT_Info[3].PMActivePower.all = 4200;
	ShowRecSerValue(0x82, 0x2036, (uint16) (gAMT_Info[4].PMActivePower.all));
	usleep(20000);

	//
	//gAMT_Info[3].PMEnergy.all = 8000;
	ShowRecSerValue2(0x82, 0x2550, (uint32) (gAMT_Info[4].PMEnergy.all));
	usleep(20000);

	//
	//gAMT_Info[3].PM_harmonics_V.all = 10;
	ShowRecSerValue(0x82, 0x2038, (uint16) (gAMT_Info[4].PMAPhaseVolt.all));
	usleep(20000);

	//
	//gAMT_Info[3].PM_harmonics_I.all = 9;
	ShowRecSerValue(0x82, 0x2039, (uint16) (gAMT_Info[4].PMAPhaseCur.all));
	usleep(20000);

	//
	//gAMT_Info[3].PMPF.all = 10;
	ShowRecSerValue(0x82, 0x2037, (uint16) (gAMT_Info[4].PMPF.all));
	usleep(20000);

	//
	//gAMT_Info[3].PMFq.all = 520;
	ShowRecSerValue(0x82, 0x203B, (uint16) (gAMT_Info[4].PMFq.all));
	usleep(20000);


	//////////////////////////////////////////
}


void UpdateUI_Page114(void) //ÏêÏ¸¸ºÔØÐÅÏ¢
{
	//gAMT_Info[0].PMAPhaseVolt.all = 2200;
	ShowRecSerValue(0x82, 0x2046, (uint16) (gAMT_Info[8].PMAPhaseVolt.all));
	usleep(20000);									//

	//gAMT_Info[0].PMBPhaseVolt.all = 2200;
	ShowRecSerValue(0x82, 0x2047, (uint16) (gAMT_Info[8].PMBPhaseVolt.all));
	usleep(20000);									//

	//gAMT_Info[0].PMCPhaseVolt.all = 2200;
	ShowRecSerValue(0x82, 0x2048, (uint16) (gAMT_Info[8].PMCPhaseVolt.all));
	usleep(20000);									//

	//
	//gAMT_Info[0].PMAPhaseCur.all = 200;
	ShowRecSerValue(0x82, 0x2049, (uint16) (gAMT_Info[8].PMAPhaseCur.all));
	usleep(20000);									//

	//gAMT_Info[0].PMBPhaseCur.all = 200;
	ShowRecSerValue(0x82, 0x204A, (uint16) (gAMT_Info[8].PMBPhaseCur.all));
	usleep(20000);									//

	//gAMT_Info[0].PMCPhaseCur.all = 200;
	ShowRecSerValue(0x82, 0x204B, (uint16) (gAMT_Info[8].PMCPhaseCur.all));
	usleep(20000);									//

	//gAMT_Info[0].PMActivePower.all = 4200;
	ShowRecSerValue(0x82, 0x204C, (uint16) (gAMT_Info[8].PMActivePower.all));
	usleep(20000);									//

	//gAMT_Info[0].PMIddlePower.all = 200;
	ShowRecSerValue(0x82, 0x204D, (uint16) (gAMT_Info[8].PMIddlePower.all));
	usleep(20000);									//

	//gAMT_Info[0].PM_harmonics_V.all = 10;
	ShowRecSerValue(0x82, 0x204E, (uint16) (gAMT_Info[8].PM_harmonics_V.all));
	usleep(20000);									//

	//gAMT_Info[0].PM_harmonics_I.all = 9;
	ShowRecSerValue(0x82, 0x204F, (uint16) (gAMT_Info[8].PM_harmonics_I.all));
	usleep(20000);									//

	//gAMT_Info[0].PMPF.all = 10;
	ShowRecSerValue(0x82, 0x2050, (uint16) (gAMT_Info[8].PMPF.all));
	usleep(20000);									//

	//gAMT_Info[0].PMFq.all = 510;
	ShowRecSerValue(0x82, 0x2051, (uint16) (gAMT_Info[8].PMFq.all));
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x212A, (uint16) (gAMT_Info[8].PMActivePower.all + gAMT_Info[8].PMIddlePower.all));
	usleep(20000);

	//
	ShowRecSerValue2(0x82, 0x2560, (uint32) (gAMT_Info[8].PMEnergy.all));
	usleep(20000);
} ///////////////////////////////////////////////////////////////


void UpdateUI_Page72(void) //¹ÊÕÏÐÅÏ¢
{
	uchar			String[4];
	char			str[8] =
		{
		0
		};

	///////////////////////////////pcs FAULT
	String[0]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable0.all >> 4) & 0xf;
	String[1]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable0.all) & 0xf;
	String[2]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable1.all >> 4) & 0xf;
	String[3]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable1.all) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x101C, str);
	usleep(20000);

	//
	String[0]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable2.all >> 4) & 0xf;
	String[1]			= (MonitorAck_BEG[DCModeNum - 1].Err_AC.INFYBit.StateTable2.all) & 0xf;
	String[2]			= (0) & 0xf;
	String[3]			= (0) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x101E, str);
	usleep(20000);

	//
	String[0]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable0.all >> 4) & 0xf;
	String[1]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable0.all) & 0xf;
	String[2]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable1.all >> 4) & 0xf;
	String[3]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable1.all) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x1028, str);
	usleep(20000);

	String[0]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable2.all >> 4) & 0xf;
	String[1]			= (MonitorAck_BEG[DCModeNum - 1].Err.INFYBit.StateTable2.all) & 0xf;
	String[2]			= (0) & 0xf;
	String[3]			= (0) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x102A, str);
	usleep(20000);

	//ACDC
	String[0]			= (OBD_ACDC_Sta1 >> 4) & 0xf;
	String[1]			= (OBD_ACDC_Sta1) & 0xf;
	String[2]			= (OBD_ACDC_Sta2 >> 4) & 0xf;
	String[3]			= (OBD_ACDC_Sta2) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x1020, str);
	usleep(20000);

	//
	//
	/////////////////////////////bms FAULT
	//gBMSInfo.Error		= 0x9f;
	String[0]			= (gBMSInfo.System_downpower >> 12) & 0xf;
	String[1]			= (gBMSInfo.System_downpower >> 8) & 0xf;
	String[2]			= (gBMSInfo.System_downpower >> 4) & 0xf;
	String[3]			= (gBMSInfo.System_downpower) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);


	//
	ShowRecSerStrings(0x82, 0x102C, str);
	usleep(20000);


	/////////////////////////////grid FAULT
	//gAMT_Info[0].grid_FAULT.all = 0x9f9f;
	String[0]			= (gBMSInfo.System_error3 >> 4) & 0xf;
	String[1]			= (gBMSInfo.System_error3) & 0xf;
	String[2]			= (gBMSInfo.System_error2 >> 4) & 0xf;
	String[3]			= (gBMSInfo.System_error2) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);


	//
	ShowRecSerStrings(0x82, 0x1022, str);
	usleep(20000);

	/////////////////////////////load FAULT
	//gAMT_Info[0].load_FAULT.all = 0x8f8f;
	String[0]			= (gBMSInfo.System_almrm2 >> 4) & 0xf;
	String[1]			= (gBMSInfo.System_almrm2) & 0xf;
	String[2]			= (gBMSInfo.System_error >> 4) & 0xf;
	String[3]			= (gBMSInfo.System_error) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);


	//
	ShowRecSerStrings(0x82, 0x102E, str);
	usleep(20000);

	/////////////////////////////STA
	//MonitorAck_BEG[0].DC_Sta1.all = 0x2cab;
	//ACDCmokai
	String[0]			= (OBD_ACDC_Sta3 >> 4) & 0xf;
	String[1]			= (OBD_ACDC_Sta3) & 0xf;
	String[2]			= (BMS_fual_max >> 4) & 0xf;
	String[3]			= (BMS_fual_max) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x1024, str);
	usleep(20000);

	//
	//MonitorAck_BEG[0].DC_Sta2.all = 0x4cab;
	String[0]			= (gBMSInfo.Protection.all >> 12) & 0xf;
	String[1]			= (gBMSInfo.Protection.all >> 8) & 0xf;
	String[2]			= (gBMSInfo.Protection.all >> 4) & 0xf;
	String[3]			= (gBMSInfo.Protection.all) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x1030, str);
	usleep(20000);

	String[0]			= (gBMSInfo.Alarm.all >> 4) & 0xf;
	String[1]			= (gBMSInfo.Alarm.all) & 0xf;
	String[2]			= (gBMSInfo.Alarm.all >> 4) & 0xf;
	String[3]			= (gBMSInfo.Alarm.all) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x1026, str);
	usleep(20000);

	//gBMSInfo.BMS_Sta	= 0x8f;
	String[0]			= gBMSInfo.Error >> 4;
	String[1]			= gBMSInfo.Error & 0xf;
	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	String[0]			= gBMSInfo.BMS_Sta >> 4;
	String[1]			= gBMSInfo.BMS_Sta & 0xf;
	str[2]				= HexToChar(String[0]);
	str[3]				= HexToChar(String[1]);

	//
	ShowRecSerStrings(0x82, 0x1032, str);
	usleep(20000);

	/////////////////////////////
}


void UpdateUI_Page73_74_bms(void) //BMS
{
	uchar			String[4];
	char			str[8] =
		{
		0
		};

	//
	//gBMSInfo.BMSVolt.all = 6669;
	//gBMSInfo.BMSCur.all = 20;
	//gBMSInfo.SOC		= 88;
	//gBMSInfo.BMS_CELL_MAX.all = 3333;
	//gBMSInfo.BMS_CELL_MIN.all = 3222;
	//gBMSInfo.BMS_TEMP_MAX.all = 288;
	//gBMSInfo.BMS_TEMP_MIN.all = 266;
	//gBMSInfo.RON.all	= 9999;
	//gBMSInfo.BMSCur_dischge_max.all = 2000;
	//gBMSInfo.BMSCur_chager_max.all = 1000;
	//
	ShowRecSerValue(0x82, 0x203C, gBMSInfo.BMSVolt.all);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x203D, gBMSInfo.BMSCur.all - 30000);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x203E, gBMSInfo.SOC);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x203F, gBMSInfo.BMS_CELL_MAX.all);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2040, gBMSInfo.BMS_CELL_MIN.all);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2041, gBMSInfo.BMS_TEMP_MAX.all - 1000);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2042, gBMSInfo.BMS_TEMP_MIN.all - 1000);
	usleep(20000);
	ShowRecSerValue(0x82, 0x2A00, gBMSInfo.BMS_CELL_MAX_add.all);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2A01, gBMSInfo.BMS_CELL_MIN_add.all);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2A02, gBMSInfo.BMS_TEMP_MAX_add.all);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2A03, gBMSInfo.BMS_TEMP_MIN_add.all);
	usleep(20000);

	//
	//String[0]			= (gBMSInfo.Protection.all >> 12) & 0xf;
	//String[1]			= (gBMSInfo.Protection.all >> 8) & 0xf;
	//String[2]			= (gBMSInfo.Protection.all >> 4) & 0xf;
	//String[3]			= (gBMSInfo.Protection.all) & 0xf;
	//str[0]				= HexToChar(String[0]);
	//str[1]				= HexToChar(String[1]);
	//str[2]				= HexToChar(String[2]);
	//str[3]				= HexToChar(String[3]);
	//
	if (gBMSInfo.RON.all >= 1000)
		{
		ShowRecSerValue(0x82, 0x2043, (9999));
		}
	else 
		{
		ShowRecSerValue(0x82, 0x2043, (gBMSInfo.RON.all * 10));
		}

	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2044, gBMSInfo.BMSCur_dischge_max.all - 30000);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2045, gBMSInfo.BMSCur_chager_max.all - 30000);
	usleep(20000);

	//gBMSInfo.System_error = 0xFF;
	String[0]			= gBMSInfo.Error >> 4;
	String[1]			= gBMSInfo.Error & 0xf;
	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	String[0]			= gBMSInfo.BMS_Sta >> 4;
	String[1]			= gBMSInfo.BMS_Sta & 0xf;
	str[2]				= HexToChar(String[0]);
	str[3]				= HexToChar(String[1]);

	//
	ShowRecSerStrings(0x82, 0x1034, str);
	usleep(20000);

	//
	UpdateUI_Cell_Temp_B2408Page();


}


void UpdateUI_Page73(uint8_t Page) //µ¥ÌåµçÑ¹
{
	uint8_t 		i, j;

	uint16_t		All_page = 0;

	All_page			= Page * 40;

	//
	for (j = 0; j < 10; j++)
		{
		for (i = 0; i < 4; i++)
			{
			ShowRecSerValue(0x82, (0x4050 + (i * 2) +j * 16), (i + j * 4 + All_page));

			//gCell_Info.BMSCell_Volt[i + j * 4 + All_page].all = 3330 + i;
			ShowRecSerValue(0x82, (0x4051 + (i * 2) +j * 16), gCell_Info.BMSCell_Volt[i + j * 4 + All_page].all);
			usleep(20000);
			}
		}
}


void UpdateUI_Page74(uint8_t Page) //µ¥ÌåÎÂ¶È
{
	uint8_t 		i, j;
	uint16_t		All_page = 0;

	All_page			= Page * 40;				//

	for (j = 0; j < 10; j++)
		{
		for (i = 0; i < 4; i++)
			{
			ShowRecSerValue(0x82, (0x4150 + (i * 2) +j * 16), (i + j * 4 + All_page));

			//gCell_Info.BMSCell_Temp[i + j * 4 + All_page].all = 250 + i;
			ShowRecSerValue(0x82, (0x4151 + (i * 2) +j * 16), gCell_Info.BMSCell_Temp[i + j * 4 + All_page].all);
			usleep(20000);
			}
		}
}


//
void UpdateUI_Cell_Temp_B2408Page(void) //µ¥ÌåÐÅÏ¢
{
	uint8_t 		i;

	//
	for (i = 0; i < 0x87; i++)
		{
		ShowRecSerValue(0x82, (0xE000 + i), gCell_Info.BMSCell_Volt[i].all);
		usleep(20000);
		}

	for (i = 0x88; i < 181; i++)
		{
		ShowRecSerValue(0x82, (0xE000 + i), gCell_Info.BMSCell_Volt[i - 1].all);
		usleep(20000);
		}

	for (i = 0; i < 15; i++)
		{
		ShowRecSerValue(0x82, (0xE0B5 + i), gCell_Info.BMSCell_Temp[i].all);
		usleep(20000);
		}
}


void UpdateUI_Page75(void) //µçÍøÐÅÏ¢
{
	//gAMT_Info[0].PMAPhaseVolt.all = 2200;
	ShowRecSerValue(0x82, 0x2046, (uint16) (gAMT_Info[5].PMAPhaseVolt.all));
	usleep(20000);									//

	//gAMT_Info[0].PMBPhaseVolt.all = 2200;
	ShowRecSerValue(0x82, 0x2047, (uint16) (gAMT_Info[5].PMBPhaseVolt.all));
	usleep(20000);									//

	//gAMT_Info[0].PMCPhaseVolt.all = 2200;
	ShowRecSerValue(0x82, 0x2048, (uint16) (gAMT_Info[5].PMCPhaseVolt.all));
	usleep(20000);									//

	//
	//gAMT_Info[0].PMAPhaseCur.all = 200;
	ShowRecSerValue(0x82, 0x2049, (uint16) (gAMT_Info[5].PMAPhaseCur.all));
	usleep(20000);									//

	//gAMT_Info[0].PMBPhaseCur.all = 200;
	ShowRecSerValue(0x82, 0x204A, (uint16) (gAMT_Info[5].PMBPhaseCur.all));
	usleep(20000);									//

	//gAMT_Info[0].PMCPhaseCur.all = 200;
	ShowRecSerValue(0x82, 0x204B, (uint16) (gAMT_Info[5].PMCPhaseCur.all));
	usleep(20000);									//

	//gAMT_Info[0].PMActivePower.all = 4200;
	ShowRecSerValue(0x82, 0x204C, (uint16) (gAMT_Info[5].PMActivePower.all));
	usleep(20000);									//

	//gAMT_Info[0].PMIddlePower.all = 200;
	ShowRecSerValue(0x82, 0x204D, (uint16) (gAMT_Info[5].PMIddlePower.all));
	usleep(20000);									//

	//gAMT_Info[0].PM_harmonics_V.all = 10;
	ShowRecSerValue(0x82, 0x204E, (uint16) (gAMT_Info[5].PM_harmonics_V.all));
	usleep(20000);									//

	//gAMT_Info[0].PM_harmonics_I.all = 9;
	ShowRecSerValue(0x82, 0x204F, (uint16) (gAMT_Info[5].PM_harmonics_I.all));
	usleep(20000);									//

	//gAMT_Info[0].PMPF.all = 10;
	ShowRecSerValue(0x82, 0x2050, (uint16) (gAMT_Info[5].PMPF.all));
	usleep(20000);									//

	//gAMT_Info[0].PMFq.all = 510;
	ShowRecSerValue(0x82, 0x2051, (uint16) (gAMT_Info[5].PMFq.all));
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x212A, (uint16) (gAMT_Info[5].PMActivePower.all + gAMT_Info[5].PMIddlePower.all));
	usleep(20000);

	//
	ShowRecSerValue2(0x82, 0x2560, (uint32) (gAMT_Info[5].PMEnergy.all));
	usleep(20000);
} ///////////////////////////////////////////////////////////////


void UpdateUI_Page65(void) //PGÐÅÏ¢
{
	uchar			String[4];

	char			str[8] =
		{
		0
		};

	//
	gWL_Info.WSys_sta.all = 0x8f8F;
	String[0]			= (gWL_Info.WSys_sta.all >> 12) & 0xf;
	String[1]			= (gWL_Info.WSys_sta.all >> 8) & 0xf;
	String[2]			= (gWL_Info.WSys_sta.all >> 4) & 0xf;
	String[3]			= gWL_Info.WSys_sta.all & 0xf;
	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	//
	ShowRecSerStrings(0x82, 0x103C, str);
	usleep(20000);

	//
	gWL_Info.WVolt.all	= 0;
	gWL_Info.WCur.all	= 0;

	//
	ShowRecSerValue(0x82, 0x2052, gWL_Info.WVolt.all);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2053, gWL_Info.WCur.all);
	usleep(20000);

	//
	//
	//
	gWL_Info.LSys_sta.all = 0x8f8F;
	String[0]			= (gWL_Info.LSys_sta.all >> 12) & 0xf;
	String[1]			= (gWL_Info.LSys_sta.all >> 8) & 0xf;
	String[2]			= (gWL_Info.LSys_sta.all >> 4) & 0xf;
	String[3]			= gWL_Info.LSys_sta.all & 0xf;
	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	//
	ShowRecSerStrings(0x82, 0x103E, str);
	usleep(20000);

	//gWL_Info.LVolt.all	= 6669;
	//gWL_Info.LCur.all	= 20;
	//
	ShowRecSerValue(0x82, 0x2054, gWL_Info.LVolt.all);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2055, gWL_Info.LCur.all);
	usleep(20000);

	/////////////////////////////////////////////////////////////////
	String[0]			= (gMonitorPG[0].Err_AC.all >> 12) & 0xf;
	String[1]			= (gMonitorPG[0].Err_AC.all >> 8) & 0xf;
	String[2]			= (gMonitorPG[0].Err_AC.all >> 4) & 0xf;
	String[3]			= (gMonitorPG[0].Err_AC.all) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x1040, str);
	usleep(20000);

	gMonitorPG[0].Module_AC_AVolt.all =
		 gAMT_Info[6].PMAPhaseVolt.all / 3 + gAMT_Info[6].PMBPhaseVolt.all / 3 + gAMT_Info[6].PMCPhaseVolt.all / 3;

	//
	ShowRecSerValue(0x82, 0x2056, gMonitorPG[0].Module_AC_AVolt.all);
	usleep(20000);

	gMonitorPG[0].Module_AC_ACur.all =
		 gAMT_Info[6].PMAPhaseCur.all + gAMT_Info[6].PMBPhaseCur.all + gAMT_Info[6].PMCPhaseCur.all;

	//
	ShowRecSerValue(0x82, 0x2057, gMonitorPG[0].Module_AC_ACur.all);
	usleep(20000);

	gMonitorPG[0].Module_AC_AllP.all = gAMT_Info[6].PMActivePower.all;

	//
	ShowRecSerValue(0x82, 0x2058, gMonitorPG[0].Module_AC_AllP.all / 1000);
	usleep(20000);

	String[0]			= (gMonitorPG[0].Err.all >> 12) & 0xf;
	String[1]			= (gMonitorPG[0].Err.all >> 8) & 0xf;
	String[2]			= (gMonitorPG[0].Err.all >> 4) & 0xf;
	String[3]			= (gMonitorPG[0].Err.all) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x1042, str);
	usleep(20000);

	///////////////////////////////////////////////////////////////
	//
	gDEG_Info.DEGSys_sta.all = SmartEnerg_io.IN_2;
	String[0]			= (gDEG_Info.DEGSys_sta.all >> 12) & 0xf;
	String[1]			= (gDEG_Info.DEGSys_sta.all >> 8) & 0xf;
	String[2]			= (gDEG_Info.DEGSys_sta.all >> 4) & 0xf;
	String[3]			= gDEG_Info.DEGSys_sta.all & 0xf;
	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	//
	ShowRecSerStrings(0x82, 0x1044, str);
	usleep(20000);

	gDEG_Info.DEGVolt.all =
		 (uint16) (gAMT_Info[5].PMAPhaseVolt.all / 3 + gAMT_Info[5].PMBPhaseVolt.all / 3 + gAMT_Info[5].PMCPhaseVolt.all / 3);
	gDEG_Info.DEGCur.all =
		 (uint16) (gAMT_Info[5].PMAPhaseCur.all + gAMT_Info[5].PMBPhaseCur.all + gAMT_Info[5].PMCPhaseCur.all);

	//
	ShowRecSerValue(0x82, 0x2059, gDEG_Info.DEGVolt.all);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x205A, gDEG_Info.DEGCur.all);
	usleep(20000);

	//
	gDEG_Info.DEG_REAL.all =
		 (SmartEnerg_io.IN_1 + SmartEnerg_io.IN_2 * 2 + SmartEnerg_io.IN_3 * 4 + SmartEnerg_io.IN_4 * 8);

	//sys_info			= gBMSInfo.Protection.all;
	gDEG_Info.DEG_SYS.all =
		 (gSmartEnerg_PGsys.SmartEnergy_WLchager << 12) + (gSmartEnerg_PGsys.SmartEnergy_DEGchager << 8) + (gSmartEnerg_sys.SmartEnergy_essrun << 4) +gSmartEnerg_sys.SmartEnergy_gridrun;

	//gDEG_Info.DEG_SYS.all = 0x8f1F;
	String[0]			= (gDEG_Info.DEG_SYS.all >> 12) & 0xf;
	String[1]			= (gDEG_Info.DEG_SYS.all >> 8) & 0xf;
	String[2]			= (gDEG_Info.DEG_SYS.all >> 4) & 0xf;
	String[3]			= gDEG_Info.DEG_SYS.all & 0xf;
	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	//
	ShowRecSerStrings(0x82, 0x1046, str);
	usleep(20000);

	//
	gDEG_Info.DEG_CONFIG.all = (gBMSInfo.BMS_Sta << 8) + (SmartEnerg_save << 4) +SmartEnerg_io.IN_3; //Ë«Ô´¿ØÖÆ
	String[0]			= (gDEG_Info.DEG_CONFIG.all >> 12) & 0xf;
	String[1]			= (gDEG_Info.DEG_CONFIG.all >> 8) & 0xf;
	String[2]			= (gDEG_Info.DEG_CONFIG.all >> 4) & 0xf;
	String[3]			= gDEG_Info.DEG_CONFIG.all & 0xf;
	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	//
	ShowRecSerStrings(0x82, 0x1048, str);
	usleep(20000);

	//
	//
	//gDEG_Info.DEG_REAL.all = 0x8f0F;
	String[0]			= (gDEG_Info.DEG_REAL.all >> 12) & 0xf;
	String[1]			= (gDEG_Info.DEG_REAL.all >> 8) & 0xf;
	String[2]			= (gDEG_Info.DEG_REAL.all >> 4) & 0xf;
	String[3]			= gDEG_Info.DEG_REAL.all & 0xf;
	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	//
	ShowRecSerStrings(0x82, 0x104A, str);
	usleep(20000);

}


void UpdateUI_Page78(void)
{
	uchar			String[4];

	//
	char			str[8] =
		{
		0
		};
	char			str1[8] =
		{
		0
		};
	char			str2[8] =
		{
		0
		};
	char			str3[8] =
		{
		0
		};
	char			str4[8] =
		{
		0
		};
	char			str5[8] =
		{
		0
		};
	char			str6[8] =
		{
		0
		};

	///AC_info
	String[0]			= (gMonitorPG[0].Err_AC.all >> 12) & 0xf;
	String[1]			= (gMonitorPG[0].Err_AC.all >> 8) & 0xf;
	String[2]			= (gMonitorPG[0].Err_AC.all >> 4) & 0xf;
	String[3]			= (gMonitorPG[0].Err_AC.all) & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x110C, str);
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2108, (uint16) (gMonitorPG[0].Module_AC_AllP.all / 100000));
	usleep(20000);

	//
	ShowRecSerValue(0x82, 0x2109, (uint16) (Time));
	Time++;

	if (Time >= 999)
		Time = 0;

	usleep(20000);

	//
	gMonitorPG[0].Temp	= PMMsgPars[7].PMHarmWaveDistData.all;

	ShowRecSerValue(0x82, 0x210A, 
		(uint16) (gMonitorPG[0].Temp / 1));
	usleep(20000);

	////////////////////////////////////////////A
	gMonitorPG[0].Module_AC_AVolt.all = 380000;
	ShowRecSerValue(0x82, 0x210B, (uint16) (gMonitorPG[0].Module_AC_AVolt.all / 1000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_AVolt.all = 220000;
	ShowRecSerValue(0x82, 0x210C, (uint16) (gMonitorPG[0].Module_AC_AVolt.all / 1000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_ACur.all = 20000;
	ShowRecSerValue(0x82, 0x210D, (uint16) (gMonitorPG[0].Module_AC_ACur.all / 1000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_APower.all = 440000;
	ShowRecSerValue(0x82, 0x210E, (uint16) (gMonitorPG[0].Module_AC_APower.all / 100000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_LAPower.all = 45000;
	ShowRecSerValue(0x82, 0x210F, (uint16) (gMonitorPG[0].Module_AC_LAPower.all / 100000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_HZ.all = 50000;
	ShowRecSerValue(0x82, 0x2110, (uint16) (gMonitorPG[0].Module_AC_HZ.all / 1000));
	usleep(20000);

	////////////////////////////////////////////B
	gMonitorPG[0].Module_AC_BVolt.all = 380000;
	ShowRecSerValue(0x82, 0x2111, (uint16) (gMonitorPG[0].Module_AC_BVolt.all / 1000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_BVolt.all = 220000;
	ShowRecSerValue(0x82, 0x2112, (uint16) (gMonitorPG[0].Module_AC_BVolt.all / 1000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_BCur.all = 20000;
	ShowRecSerValue(0x82, 0x2113, (uint16) (gMonitorPG[0].Module_AC_BCur.all / 1000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_BPower.all = 440000;
	ShowRecSerValue(0x82, 0x2114, (uint16) (gMonitorPG[0].Module_AC_BPower.all / 100000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_LBPower.all = 45000;
	ShowRecSerValue(0x82, 0x2115, (uint16) (gMonitorPG[0].Module_AC_LBPower.all / 100000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_HZ.all = 50000;
	ShowRecSerValue(0x82, 0x2116, (uint16) (gMonitorPG[0].Module_AC_HZ.all / 1000));
	usleep(20000);

	////////////////////////////////////////////C
	gMonitorPG[0].Module_AC_CVolt.all = 380000;
	ShowRecSerValue(0x82, 0x2117, (uint16) (gMonitorPG[0].Module_AC_CVolt.all / 1000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_CVolt.all = 220000;
	ShowRecSerValue(0x82, 0x2118, (uint16) (gMonitorPG[0].Module_AC_CVolt.all / 1000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_CCur.all = 20000;
	ShowRecSerValue(0x82, 0x2119, (uint16) (gMonitorPG[0].Module_AC_CCur.all / 1000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_CPower.all = 440000;
	ShowRecSerValue(0x82, 0x211A, (uint16) (gMonitorPG[0].Module_AC_CPower.all / 100000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_LCPower.all = 45000;
	ShowRecSerValue(0x82, 0x211B, (uint16) (gMonitorPG[0].Module_AC_LCPower.all / 100000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_HZ.all = 50000;
	ShowRecSerValue(0x82, 0x211C, (uint16) (gMonitorPG[0].Module_AC_HZ.all / 1000));
	usleep(20000);

	////////////////////////////////////////////ALL
	gMonitorPG[0].Module_AC_CVolt.all = 380000;
	ShowRecSerValue(0x82, 0x211D, (uint16) (gMonitorPG[0].Module_AC_CVolt.all / 1000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_CVolt.all = 220000;
	ShowRecSerValue(0x82, 0x211E, (uint16) (gMonitorPG[0].Module_AC_CVolt.all / 1000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_CCur.all = 20000;
	ShowRecSerValue(0x82, 0x211F, (uint16) (gMonitorPG[0].Module_AC_CCur.all / 1000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_CPower.all = 440000;
	ShowRecSerValue(0x82, 0x2120, (uint16) (gMonitorPG[0].Module_AC_CPower.all / 100000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_LCPower.all = 45000;
	ShowRecSerValue(0x82, 0x2121, (uint16) (gMonitorPG[0].Module_AC_LCPower.all / 100000));
	usleep(20000);									//
	gMonitorPG[0].Module_AC_HZ.all = 50000;
	ShowRecSerValue(0x82, 0x2122, (uint16) (gMonitorPG[0].Module_AC_HZ.all / 1000));
	usleep(20000);

	//////////////////////////////////////////
	//HV
	gMonitorPG[0].ModuleVoltOut.all = 650000;
	ShowRecSerValue(0x82, 0x2123, (uint16) (gMonitorPG[0].ModuleVoltOut.all / 100));
	usleep(20000);									//
	gMonitorPG[0].ModuleCurOut.all = 50000;
	ShowRecSerValue(0x82, 0x2124, (uint16) (gMonitorPG[0].ModuleCurOut.all / 100));
	usleep(20000);
	gMonitorPG[0].ModuleCurOut.all = 50000;
	ShowRecSerValue(0x82, 0x2125, 
		(uint16) ((gMonitorPG[0].ModuleCurOut.all / 10000) * (gMonitorPG[0].ModuleVoltOut.all / 10000)));
	usleep(20000);									//LV
	gMonitorPG[0].ModuleVoltOut.all = 650000;
	ShowRecSerValue(0x82, 0x2126, (uint16) (gMonitorPG[0].ModuleVoltOut.all / 100));
	usleep(20000);									//
	gMonitorPG[0].ModuleCurOut.all = 50000;
	ShowRecSerValue(0x82, 0x2127, (uint16) (gMonitorPG[0].ModuleCurOut.all / 100));
	usleep(20000);
	gMonitorPG[0].ModuleCurOut.all = 50000;
	ShowRecSerValue(0x82, 0x2128, 
		(uint16) ((gMonitorPG[0].ModuleCurOut.all / 10000) * (gMonitorPG[0].ModuleVoltOut.all / 10000)));
	usleep(20000);

	//////////////////////////////////////////
	String[0]			= (gMonitorPG[0].Err.all >> 12) & 0xf;
	String[1]			= (gMonitorPG[0].Err.all >> 8) & 0xf;
	String[2]			= (gMonitorPG[0].Err.all >> 4) & 0xf;
	String[3]			= (gMonitorPG[0].Err.all) & 0xf;
	str1[0] 			= ' ';
	str1[1] 			= ' ';
	str1[2] 			= ' ';
	str1[3] 			= HexToChar(String[0]);
	str2[0] 			= ' ';
	str2[1] 			= ' ';
	str2[2] 			= ' ';
	str2[3] 			= HexToChar(String[1]);
	str3[0] 			= ' ';
	str3[1] 			= ' ';
	str3[2] 			= ' ';
	str3[3] 			= HexToChar(String[2]);
	str4[0] 			= ' ';
	str4[1] 			= ' ';
	str4[2] 			= ' ';
	str4[3] 			= HexToChar(String[3]);
	ShowRecSerStrings(0x82, 0x110E, str1);
	usleep(20000);
	ShowRecSerStrings(0x82, 0x1110, str2);
	usleep(20000);
	ShowRecSerStrings(0x82, 0x1112, str3);
	usleep(20000);
	ShowRecSerStrings(0x82, 0x1114, str4);
	usleep(20000);
	ShowRecSerStrings(0x82, 0x1116, "FFFF");
	usleep(20000);
	ShowRecSerStrings(0x82, 0x1118, "FFFF");
	usleep(20000);

	//////////////////////////////////////////
	ShowRecSerValue(0x82, 0x2129, 
		(uint16) (gMonitorPG[0].Temp / 1));
	usleep(20000);									//

	//
} ///////////////////////////////////////
///////////////////////////////////////////////////////////////
//MAX12×Ö·û
///////////////////////////////////////////////////////////////


void UpdateUI_Page81(void)
{
	char			str[32];

	//
	HMI_sys_ini();

	if (SYS_SET_Page81 == 0)
		{
		if (SYS_SET_time_flg == 0)
			{
			ShowRecSerValue(0x82, 0x1420, (uint16) (0));
			usleep(20000);
			ShowRecSerStrings_ERR(5, 0x82, 0x1120, "DATE:");
			usleep(20000);
			ShowRecSerStrings_ERR(9, 0x82, 0x1130, get_DBsys_time_data(str));
			usleep(20000);
			ShowRecSerStrings_ERR(5, 0x82, 0x1140, "TIME:");
			usleep(20000);
			ShowRecSerStrings_ERR(8, 0x82, 0x1150, get_DBsys_time_time(str));
			usleep(20000);
			}

		}

	ShowRecSerValue(0x82, 0x2570, SYS_SET_Charg_Power);
	usleep(20000);

	ShowRecSerValue(0x82, 0x2580, SYS_SET_PCS_Num);
	usleep(20000);
}


///////////////////////////////////////Query_info
void UpdateUI_Page82(void)
{
	//Bms_report		gbms_report_cs;
	//Day_EnergyFee	gDay_EnergyFee_cs;
	//Day_power		gDay_power_cs;
	char			temp[32], temp_time[32];

	//
	if (Query_info_BOM == 1)
		{
		switch (Query_info_sta)
			{
			case 0:
				//gbms_report_cs.id = Enquiries_Page82 * 2;
				Page_Query_num = Enquiries_Page82 * 2;
				ShowRecSerValue(0x82, 0x1420, (uint16) (Page_Query_num));
				usleep(20000);

				//
				//Querybms_report(gbms_report_cs);
				Page_Query_info = 1;

				while (Page_Query_info != 0)
					{
					usleep(100000);
					printf("Page_Query_info: %d\n", Page_Query_info);
					usleep(100000);
					}

				//
				metis_strftime(atoi(Query_info2), temp);

				//printf("TIME: %s\n", temp);
				ShowRecSerStrings_ERR(5, 0x82, 0x12A0, "DATE:");
				usleep(20000);
				temp[10] = 0x00;
				ShowRecSerStrings_ERR(10, 0x82, 0x12B0, 
					temp);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x12E0, "TIME:");
				usleep(20000);

				//
				temp_time[0] = temp[11];
				temp_time[1] = temp[12];
				temp_time[2] = temp[13];
				temp_time[3] = temp[14];
				temp_time[4] = temp[15];
				temp_time[5] = temp[16];
				temp_time[6] = temp[17];
				temp_time[7] = temp[18];
				temp_time[8] = temp[19];
				temp_time[9] = temp[20];
				ShowRecSerStrings_ERR(9, 0x82, 0x12F0, 
					temp_time);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x1320, "isoc:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info3), 0x82, 0x1330, 
					Query_info3);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x1320, "isoc:");
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x1360, "isoh:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info4), 0x82, 0x1370, 
					Query_info4);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x13A0, "isoE:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info5), 0x82, 0x13B0, 
					Query_info5);
				usleep(20000);
				ShowRecSerStrings_ERR(7, 0x82, 0x13E0, "iCycle:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info6), 0x82, 0x13F0, 
					Query_info6);
				usleep(20000);

				//
				Query_info_sta = 1;
				break;

			case 1:
				//gbms_report_cs.id = Enquiries_Page82 * 2 + 1;
				Page_Query_num = Enquiries_Page82 * 2 + 1;

				//Querybms_report(gbms_report_cs);
				Page_Query_info = 1;

				while (Page_Query_info != 0)
					{
					usleep(100000);
					printf("Page_Query_info: %d\n", Page_Query_info);
					usleep(100000);
					}

				metis_strftime(atoi(Query_info2), temp);

				//printf("TIME: %s\n", temp);
				ShowRecSerStrings_ERR(5, 0x82, 0x12C0, "DATE:");
				usleep(20000);
				temp[10] = 0x00;
				ShowRecSerStrings_ERR(10, 0x82, 0x12D0, 
					temp);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x1300, "TIME:");
				usleep(20000);

				//
				temp_time[0] = temp[11];
				temp_time[1] = temp[12];
				temp_time[2] = temp[13];
				temp_time[3] = temp[14];
				temp_time[4] = temp[15];
				temp_time[5] = temp[16];
				temp_time[6] = temp[17];
				temp_time[7] = temp[18];
				temp_time[8] = temp[19];
				temp_time[9] = temp[20];
				ShowRecSerStrings_ERR(9, 0x82, 0x1310, 
					temp_time);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x1340, "isoc:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info3), 0x82, 0x1350, 
					Query_info3);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x1340, "isoc:");
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x1380, "isoh:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info4), 0x82, 0x1390, 
					Query_info4);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x13C0, "isoE:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info5), 0x82, 0x13D0, 
					Query_info5);
				usleep(20000);
				ShowRecSerStrings_ERR(7, 0x82, 0x1400, "iCycle:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info6), 0x82, 0x1410, 
					Query_info6);
				usleep(20000);

				//
				Query_info_sta = 2;
				break;
			}
		}

	//
	if (Query_info_BOM == 2)
		{
		switch (Query_info_sta)
			{
			case 0:
				//gDay_EnergyFee_cs.id = Enquiries_Page82 * 2;
				Page_Query_num = Enquiries_Page82 * 2;
				ShowRecSerValue(0x82, 0x1420, (uint16) (Page_Query_num));
				usleep(20000);

				//QueryDay_EnergyFee(gDay_EnergyFee_cs);
				Page_Query_info = 2;

				while (Page_Query_info != 0)
					{
					usleep(100000);
					printf("Page_Query_info: %d\n", Page_Query_info);
					usleep(100000);
					}

				metis_strftime(atoi(Query_info2), temp);

				//printf("TIME: %s\n", temp);
				ShowRecSerStrings_ERR(5, 0x82, 0x12A0, "DATE:");
				usleep(20000);
				temp[10] = 0x00;
				ShowRecSerStrings_ERR(10, 0x82, 0x12B0, 
					temp);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x12E0, "TIME:");
				usleep(20000);

				//
				temp_time[0] = temp[11];
				temp_time[1] = temp[12];
				temp_time[2] = temp[13];
				temp_time[3] = temp[14];
				temp_time[4] = temp[15];
				temp_time[5] = temp[16];
				temp_time[6] = temp[17];
				temp_time[7] = temp[18];
				temp_time[8] = temp[19];
				temp_time[9] = temp[20];
				ShowRecSerStrings_ERR(9, 0x82, 0x12F0, 
					temp_time);
				usleep(20000);
				ShowRecSerStrings_ERR(10, 0x82, 0x1320, "ChargeFee:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info3), 0x82, 0x1330, 
					Query_info3);
				usleep(20000);
				ShowRecSerStrings_ERR(10, 0x82, 0x1320, "ChargeFee:");
				usleep(20000);
				ShowRecSerStrings_ERR(12, 0x82, 0x1360, "DisChargeFe:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info4), 0x82, 0x1370, 
					Query_info4);
				usleep(20000);
				ShowRecSerStrings_ERR(11, 0x82, 0x13A0, "ChargeEner:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info5), 0x82, 0x13B0, 
					Query_info5);
				usleep(20000);
				ShowRecSerStrings_ERR(11, 0x82, 0x13E0, "DisChaEner:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info6), 0x82, 0x13F0, 
					Query_info6);
				usleep(20000);

				//
				Query_info_sta = 1;
				break;

			case 1:
				//gDay_EnergyFee_cs.id = Enquiries_Page82 * 2 + 1;
				Page_Query_num = Enquiries_Page82 * 2 + 1;

				//QueryDay_EnergyFee(gDay_EnergyFee_cs);
				Page_Query_info = 2;

				while (Page_Query_info != 0)
					{
					usleep(100000);
					printf("Page_Query_info: %d\n", Page_Query_info);
					usleep(100000);
					}

				metis_strftime(atoi(Query_info2), temp);

				//printf("TIME: %s\n", temp);
				ShowRecSerStrings_ERR(5, 0x82, 0x12C0, "DATE:");
				usleep(20000);
				temp[10] = 0x00;
				ShowRecSerStrings_ERR(10, 0x82, 0x12D0, 
					temp);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x1300, "TIME:");
				usleep(20000);

				//
				temp_time[0] = temp[11];
				temp_time[1] = temp[12];
				temp_time[2] = temp[13];
				temp_time[3] = temp[14];
				temp_time[4] = temp[15];
				temp_time[5] = temp[16];
				temp_time[6] = temp[17];
				temp_time[7] = temp[18];
				temp_time[8] = temp[19];
				temp_time[9] = temp[20];
				ShowRecSerStrings_ERR(9, 0x82, 0x1310, 
					temp_time);
				usleep(20000);
				ShowRecSerStrings_ERR(10, 0x82, 0x1340, "ChargeFee:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info3), 0x82, 0x1350, 
					Query_info3);
				usleep(20000);
				ShowRecSerStrings_ERR(10, 0x82, 0x1340, "ChargeFee:");
				usleep(20000);
				ShowRecSerStrings_ERR(12, 0x82, 0x1380, "DisChargeFe:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info4), 0x82, 0x1390, 
					Query_info4);
				usleep(20000);
				ShowRecSerStrings_ERR(11, 0x82, 0x13C0, "ChargeEner:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info5), 0x82, 0x13D0, 
					Query_info5);
				usleep(20000);
				ShowRecSerStrings_ERR(11, 0x82, 0x1400, "DisChaEner:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info6), 0x82, 0x1410, 
					Query_info6);
				usleep(20000);

				//
				Query_info_sta = 2;
				break;
			}
		}

	//
	if (Query_info_BOM == 3)
		{
		switch (Query_info_sta)
			{
			case 0:
				//gDay_power_cs.id = Enquiries_Page82 * 2;
				Page_Query_num = Enquiries_Page82 * 2;
				ShowRecSerValue(0x82, 0x1420, (uint16) (Page_Query_num));
				usleep(20000);

				//Queryday_power(gDay_power_cs);
				Page_Query_info = 3;

				while (Page_Query_info != 0)
					{
					usleep(100000);
					printf("Page_Query_info: %d\n", Page_Query_info);
					usleep(100000);
					}

				metis_strftime(atoi(Query_info2), temp);

				//printf("TIME: %s\n", temp);
				ShowRecSerStrings_ERR(5, 0x82, 0x12A0, "DATE:");
				usleep(20000);
				temp[10] = 0x00;
				ShowRecSerStrings_ERR(10, 0x82, 0x12B0, 
					temp);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x12E0, "TIME:");
				usleep(20000);

				//
				temp_time[0] = temp[11];
				temp_time[1] = temp[12];
				temp_time[2] = temp[13];
				temp_time[3] = temp[14];
				temp_time[4] = temp[15];
				temp_time[5] = temp[16];
				temp_time[6] = temp[17];
				temp_time[7] = temp[18];
				temp_time[8] = temp[19];
				temp_time[9] = temp[20];
				ShowRecSerStrings_ERR(9, 0x82, 0x12F0, 
					temp_time);
				usleep(20000);
				ShowRecSerStrings_ERR(12, 0x82, 0x1320, "activePower:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info3), 0x82, 0x1330, 
					Query_info3);
				usleep(20000);
				ShowRecSerStrings_ERR(12, 0x82, 0x1320, "activePower:");
				usleep(20000);
				ShowRecSerStrings_ERR(10, 0x82, 0x1360, "reactiveP:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info4), 0x82, 0x1370, 
					Query_info4);
				usleep(20000);
				ShowRecSerStrings_ERR(4, 0x82, 0x13A0, "NULL");
				usleep(20000);
				ShowRecSerStrings_ERR(4, 0x82, 0x13B0, "NULL");
				usleep(20000);
				ShowRecSerStrings_ERR(4, 0x82, 0x13E0, "NULL");
				usleep(20000);
				ShowRecSerStrings_ERR(4, 0x82, 0x13F0, "NULL");
				usleep(20000);

				//
				Query_info_sta = 1;
				break;

			case 1:
				//gbms_report_cs.id = Enquiries_Page82 * 2 + 1;
				Page_Query_num = Enquiries_Page82 * 2 + 1;

				//Queryday_power(gDay_power_cs);
				Page_Query_info = 3;

				while (Page_Query_info != 0)
					{
					usleep(100000);
					printf("Page_Query_info: %d\n", Page_Query_info);
					usleep(100000);
					}

				metis_strftime(atoi(Query_info2), temp);

				//printf("TIME: %s\n", temp);
				ShowRecSerStrings_ERR(5, 0x82, 0x12C0, "DATE:");
				usleep(20000);
				temp[10] = 0x00;
				ShowRecSerStrings_ERR(10, 0x82, 0x12D0, 
					temp);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x1300, "TIME:");
				usleep(20000);

				//
				temp_time[0] = temp[11];
				temp_time[1] = temp[12];
				temp_time[2] = temp[13];
				temp_time[3] = temp[14];
				temp_time[4] = temp[15];
				temp_time[5] = temp[16];
				temp_time[6] = temp[17];
				temp_time[7] = temp[18];
				temp_time[8] = temp[19];
				temp_time[9] = temp[20];
				ShowRecSerStrings_ERR(9, 0x82, 0x1310, 
					temp_time);
				usleep(20000);
				ShowRecSerStrings_ERR(12, 0x82, 0x1340, "activePower:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info3), 0x82, 0x1350, 
					Query_info3);
				usleep(20000);
				ShowRecSerStrings_ERR(12, 0x82, 0x1340, "activePower:");
				usleep(20000);
				ShowRecSerStrings_ERR(10, 0x82, 0x1380, "reactiveP:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info4), 0x82, 0x1390, 
					Query_info4);
				usleep(20000);
				ShowRecSerStrings_ERR(4, 0x82, 0x13C0, "NULL");
				usleep(20000);
				ShowRecSerStrings_ERR(4, 0x82, 0x13D0, "NULL");
				usleep(20000);
				ShowRecSerStrings_ERR(4, 0x82, 0x1400, "NULL");
				usleep(20000);
				ShowRecSerStrings_ERR(4, 0x82, 0x1410, "NULL");
				usleep(20000);

				//
				Query_info_sta = 2;
				break;
			}
		}

	//
}


///////////////////////////////////////
void UpdateUI_Page84(void)
{
	//Ctrl_log		gctrl_log_cs;
	//Config_log		gconfig_log_cs;
	char			temp[32], temp_time[32];

	//
	if (Query_info_BOM == 1)
		{
		switch (Query_info_sta)
			{
			case 0:
				//gctrl_log_cs.id = Enquiries_Page84 * 2;
				Page_Query_num = Enquiries_Page84 * 2;
				ShowRecSerValue(0x82, 0x1420, (uint16) (Page_Query_num));
				usleep(20000);

				//Queryctrl_log(gctrl_log_cs);
				Page_Query_info = 4;

				while (Page_Query_info != 0)
					{
					usleep(100000);
					printf("Page_Query_info: %d\n", Page_Query_info);
					usleep(100000);
					}

				metis_strftime(atoi(Query_info7), temp);

				//printf("TIME: %s\n", temp);
				ShowRecSerStrings_ERR(5, 0x82, 0x12A0, "DATE:");
				usleep(20000);
				temp[10] = 0x00;
				ShowRecSerStrings_ERR(10, 0x82, 0x12B0, 
					temp);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x12E0, "TIME:");
				usleep(20000);

				//
				temp_time[0] = temp[11];
				temp_time[1] = temp[12];
				temp_time[2] = temp[13];
				temp_time[3] = temp[14];
				temp_time[4] = temp[15];
				temp_time[5] = temp[16];
				temp_time[6] = temp[17];
				temp_time[7] = temp[18];
				temp_time[8] = temp[19];
				temp_time[9] = temp[20];
				ShowRecSerStrings_ERR(9, 0x82, 0x12F0, 
					temp_time);
				usleep(20000);
				ShowRecSerStrings_ERR(10, 0x82, 0x1320, "equipName:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info3), 0x82, 0x1330, 
					Query_info3);
				usleep(20000);
				ShowRecSerStrings_ERR(10, 0x82, 0x1320, "equipName:");
				usleep(20000);
				ShowRecSerStrings_ERR(8, 0x82, 0x1360, "sigName:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info4), 0x82, 0x1370, 
					Query_info4);
				usleep(20000);
				ShowRecSerStrings_ERR(9, 0x82, 0x13A0, "oldValue:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info5), 0x82, 0x13B0, 
					Query_info5);
				usleep(20000);
				ShowRecSerStrings_ERR(9, 0x82, 0x13E0, "newValue:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info6), 0x82, 0x13F0, 
					Query_info6);
				usleep(20000);

				//
				Query_info_sta = 1;
				break;

			case 1:
				//gctrl_log_cs.id = Enquiries_Page84 * 2 + 1;
				Page_Query_num = Enquiries_Page84 * 2 + 1;

				//Queryctrl_log(gctrl_log_cs);
				Page_Query_info = 4;

				while (Page_Query_info != 0)
					{
					usleep(100000);
					printf("Page_Query_info: %d\n", Page_Query_info);
					usleep(100000);
					}

				metis_strftime(atoi(Query_info7), temp);

				//printf("TIME: %s\n", temp);
				ShowRecSerStrings_ERR(5, 0x82, 0x12C0, "DATE:");
				usleep(20000);
				temp[10] = 0x00;
				ShowRecSerStrings_ERR(10, 0x82, 0x12D0, 
					temp);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x1300, "TIME:");
				usleep(20000);

				//
				temp_time[0] = temp[11];
				temp_time[1] = temp[12];
				temp_time[2] = temp[13];
				temp_time[3] = temp[14];
				temp_time[4] = temp[15];
				temp_time[5] = temp[16];
				temp_time[6] = temp[17];
				temp_time[7] = temp[18];
				temp_time[8] = temp[19];
				temp_time[9] = temp[20];
				ShowRecSerStrings_ERR(9, 0x82, 0x1310, 
					temp_time);
				usleep(20000);
				ShowRecSerStrings_ERR(10, 0x82, 0x1340, "equipName:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info3), 0x82, 0x1350, 
					Query_info3);
				usleep(20000);
				ShowRecSerStrings_ERR(10, 0x82, 0x1340, "equipName:");
				usleep(20000);
				ShowRecSerStrings_ERR(8, 0x82, 0x1380, "sigName:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info4), 0x82, 0x1390, 
					Query_info4);
				usleep(20000);
				ShowRecSerStrings_ERR(9, 0x82, 0x13C0, "oldValue:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info5), 0x82, 0x13D0, 
					Query_info5);
				usleep(20000);
				ShowRecSerStrings_ERR(9, 0x82, 0x1400, "newValue:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info6), 0x82, 0x1410, 
					Query_info6);
				usleep(20000);

				//
				Query_info_sta = 2;
				break;
			}
		}

	//
	if (Query_info_BOM == 2)
		{
		switch (Query_info_sta)
			{
			case 0:
				//gconfig_log_cs.id = Enquiries_Page84 * 2;
				Page_Query_num = Enquiries_Page84 * 2;
				ShowRecSerValue(0x82, 0x1420, (uint16) (Page_Query_num));
				usleep(20000);

				//Queryconfig_log(gconfig_log_cs);
				Page_Query_info = 5;

				while (Page_Query_info != 0)
					{
					usleep(100000);
					printf("Page_Query_info: %d\n", Page_Query_info);
					usleep(100000);
					}

				metis_strftime(atoi(Query_info6), temp);

				//printf("TIME: %s\n", temp);
				ShowRecSerStrings_ERR(5, 0x82, 0x12A0, "DATE:");
				usleep(20000);
				temp[10] = 0x00;
				ShowRecSerStrings_ERR(10, 0x82, 0x12B0, 
					temp);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x12E0, "TIME:");
				usleep(20000);

				//
				temp_time[0] = temp[11];
				temp_time[1] = temp[12];
				temp_time[2] = temp[13];
				temp_time[3] = temp[14];
				temp_time[4] = temp[15];
				temp_time[5] = temp[16];
				temp_time[6] = temp[17];
				temp_time[7] = temp[18];
				temp_time[8] = temp[19];
				temp_time[9] = temp[20];
				ShowRecSerStrings_ERR(9, 0x82, 0x12F0, 
					temp_time);
				usleep(20000);
				ShowRecSerStrings_ERR(9, 0x82, 0x1320, "UserType:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info2), 0x82, 0x1330, 
					Query_info2);
				usleep(20000);
				ShowRecSerStrings_ERR(9, 0x82, 0x1320, "UserType:");
				usleep(20000);
				ShowRecSerStrings_ERR(11, 0x82, 0x1360, "configType:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info3), 0x82, 0x1370, 
					Query_info3);
				usleep(20000);
				ShowRecSerStrings_ERR(9, 0x82, 0x13A0, "oldValue:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info4), 0x82, 0x13B0, 
					Query_info4);
				usleep(20000);
				ShowRecSerStrings_ERR(9, 0x82, 0x13E0, "newValue:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info5), 0x82, 0x13F0, 
					Query_info5);
				usleep(20000);

				//
				Query_info_sta = 1;
				break;

			case 1:
				//gconfig_log_cs.id = Enquiries_Page84 * 2 + 1;
				Page_Query_num = Enquiries_Page84 * 2 + 1;

				//Queryconfig_log(gconfig_log_cs);
				Page_Query_info = 5;

				while (Page_Query_info != 0)
					{
					usleep(100000);
					printf("Page_Query_info: %d\n", Page_Query_info);
					usleep(100000);
					}

				metis_strftime(atoi(Query_info6), temp);

				//printf("TIME: %s\n", temp);
				ShowRecSerStrings_ERR(5, 0x82, 0x12C0, "DATE:");
				usleep(20000);
				temp[10] = 0x00;
				ShowRecSerStrings_ERR(10, 0x82, 0x12D0, 
					temp);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x1300, "TIME:");
				usleep(20000);

				//
				temp_time[0] = temp[11];
				temp_time[1] = temp[12];
				temp_time[2] = temp[13];
				temp_time[3] = temp[14];
				temp_time[4] = temp[15];
				temp_time[5] = temp[16];
				temp_time[6] = temp[17];
				temp_time[7] = temp[18];
				temp_time[8] = temp[19];
				temp_time[9] = temp[20];
				ShowRecSerStrings_ERR(9, 0x82, 0x1310, 
					temp_time);
				usleep(20000);
				ShowRecSerStrings_ERR(9, 0x82, 0x1340, "UserType:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info2), 0x82, 0x1350, 
					Query_info2);
				usleep(20000);
				ShowRecSerStrings_ERR(9, 0x82, 0x1340, "UserType:");
				usleep(20000);
				ShowRecSerStrings_ERR(11, 0x82, 0x1380, "configType:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info3), 0x82, 0x1390, 
					Query_info3);
				usleep(20000);
				ShowRecSerStrings_ERR(9, 0x82, 0x13C0, "oldValue:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info4), 0x82, 0x13D0, 
					Query_info4);
				usleep(20000);
				ShowRecSerStrings_ERR(9, 0x82, 0x1400, "newValue:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info5), 0x82, 0x1410, 
					Query_info5);
				usleep(20000);

				//
				Query_info_sta = 2;
				break;
			}
		}

}


///////////////////////////////////////
void UpdateUI_Page85(void)
{
	//Alarm			W_alarm_cs;
	char			temp[32], temp_time[32];

	//
	if (Query_info_BOM == 1)
		{
		switch (Query_info_sta)
			{
			case 0:
				//W_alarm_cs.active_alarm.id = Enquiries_Page85 * 2;
				Page_Query_num = Enquiries_Page85 * 2;
				ShowRecSerValue(0x82, 0x1420, (uint16) (Page_Query_num));
				usleep(20000);

				//QueryalarmGroups_active_alarm(W_alarm_cs);
				Page_Query_info = 6;

				while (Page_Query_info != 0)
					{
					usleep(100000);
					printf("Page_Query_info: %d\n", Page_Query_info);
					usleep(100000);
					}

				metis_strftime(atoi(Query_info6), temp);

				//printf("TIME: %s\n", temp);
				ShowRecSerStrings_ERR(5, 0x82, 0x12A0, "DATE:");
				usleep(20000);
				temp[10] = 0x00;
				ShowRecSerStrings_ERR(10, 0x82, 0x12B0, 
					temp);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x12E0, "TIME:");
				usleep(20000);

				//
				temp_time[0] = temp[11];
				temp_time[1] = temp[12];
				temp_time[2] = temp[13];
				temp_time[3] = temp[14];
				temp_time[4] = temp[15];
				temp_time[5] = temp[16];
				temp_time[6] = temp[17];
				temp_time[7] = temp[18];
				temp_time[8] = temp[19];
				temp_time[9] = temp[20];
				ShowRecSerStrings_ERR(9, 0x82, 0x12F0, 
					temp_time);
				usleep(20000);
				ShowRecSerStrings_ERR(8, 0x82, 0x1320, "EquipID:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info2), 0x82, 0x1330, 
					Query_info2);
				usleep(20000);
				ShowRecSerStrings_ERR(8, 0x82, 0x1320, "EquipID:");
				usleep(20000);
				ShowRecSerStrings_ERR(6, 0x82, 0x1360, "SigID:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info3), 0x82, 0x1370, 
					Query_info3);
				usleep(20000);
				ShowRecSerStrings_ERR(11, 0x82, 0x13A0, "AlarmLevel:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info4), 0x82, 0x13B0, 
					Query_info4);
				usleep(20000);
				ShowRecSerStrings_ERR(10, 0x82, 0x13E0, "AlarmInfo:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info5), 0x82, 0x13F0, 
					Query_info5);
				usleep(20000);

				//
				Query_info_sta = 1;
				break;

			case 1:
				//W_alarm_cs.active_alarm.id = Enquiries_Page85 * 2 + 1;
				Page_Query_num = Enquiries_Page85 * 2 + 1;

				//QueryalarmGroups_active_alarm(W_alarm_cs);
				Page_Query_info = 6;

				while (Page_Query_info != 0)
					{
					usleep(100000);
					printf("Page_Query_info: %d\n", Page_Query_info);
					usleep(100000);
					}

				metis_strftime(atoi(Query_info6), temp);

				//printf("TIME: %s\n", temp);
				ShowRecSerStrings_ERR(5, 0x82, 0x12C0, "DATE:");
				usleep(20000);
				temp[10] = 0x00;
				ShowRecSerStrings_ERR(10, 0x82, 0x12D0, 
					temp);
				usleep(20000);
				ShowRecSerStrings_ERR(5, 0x82, 0x1300, "TIME:");
				usleep(20000);

				//
				temp_time[0] = temp[11];
				temp_time[1] = temp[12];
				temp_time[2] = temp[13];
				temp_time[3] = temp[14];
				temp_time[4] = temp[15];
				temp_time[5] = temp[16];
				temp_time[6] = temp[17];
				temp_time[7] = temp[18];
				temp_time[8] = temp[19];
				temp_time[9] = temp[20];
				ShowRecSerStrings_ERR(9, 0x82, 0x1310, 
					temp_time);
				usleep(20000);
				ShowRecSerStrings_ERR(8, 0x82, 0x1340, "EquipID:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info2), 0x82, 0x1350, 
					Query_info2);
				usleep(20000);
				ShowRecSerStrings_ERR(8, 0x82, 0x1340, "EquipID:");
				usleep(20000);
				ShowRecSerStrings_ERR(6, 0x82, 0x1380, "SigID:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info3), 0x82, 0x1390, 
					Query_info3);
				usleep(20000);
				ShowRecSerStrings_ERR(11, 0x82, 0x13C0, "AlarmLevel:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info4), 0x82, 0x13D0, 
					Query_info4);
				usleep(20000);
				ShowRecSerStrings_ERR(10, 0x82, 0x1400, "AlarmInfo:");
				usleep(20000);
				ShowRecSerStrings_ERR(strlen(Query_info5), 0x82, 0x1410, 
					Query_info5);
				usleep(20000);

				//
				Query_info_sta = 2;
				break;
			}
		}

	//
}


///////////////////////////////////////
void Page204_B2408_relay(void)
{
	uchar			String[4];
	char			str[8] =
		{
		0
		};

	String[0]			= (SmartEnerg_io.IN_1) & 0xf;
	String[1]			= (SmartEnerg_io.IN_2) & 0xf;
	String[2]			= (SmartEnerg_io.IN_3) & 0xf;
	String[3]			= SmartEnerg_io.IN_4 & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x1500, str);
	usleep(20000);

	String[0]			= (SmartEnerg_io.OUT_1) & 0xf;
	String[1]			= (SmartEnerg_io.OUT_2) & 0xf;
	String[2]			= (SmartEnerg_io.OUT_3) & 0xf;
	String[3]			= SmartEnerg_io.OUT_4 & 0xf;

	str[0]				= HexToChar(String[0]);
	str[1]				= HexToChar(String[1]);
	str[2]				= HexToChar(String[2]);
	str[3]				= HexToChar(String[3]);

	ShowRecSerStrings(0x82, 0x1502, str);
	usleep(20000);

}


///////////////////////////////////////
void UpdateUI_Page95(void)
{
	BMS_sys_ini();

}


///////////////////////////////////////
void UpdateUI_Page96(void)
{
}


///////////////////////////////////////
void UpdateUI_Page97(void)
{
}


///////////////////////////////////////
void UpdateUI_Page98(void)
{
}


///////////////////////////////////////
void UpdateUI_Page99(void)
{
	SYS_sys_ini();

}


///////////////////////////////////////////////////////////////
//B2408 Í¼±ê
void UpdateSetcur_B2408_ess1(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x4000, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x4000, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_ess2(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x4002, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x4002, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_ess3(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x4004, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x4004, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_ess4(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x4006, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x4006, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_ess5(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x4008, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x4008, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_ccs(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x400A, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x400A, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_OBD(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x400C, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x400C, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_EV(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x400E, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x400E, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_CDZ(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x4010, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x4010, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_ESS(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x4012, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x4012, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_PCS(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x4014, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x4014, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_LD(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x4016, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x4016, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_DISCG(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x4018, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x4018, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_CHARGE(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x401A, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x401A, 1);			//
		usleep(20000);
		}

}


void UpdateSetcur_B2408_EER(uchar flag) //--->
{
	if (flag == 0)
		{
		//
		ShowRecSerValue(0x82, 0x401C, 0);			//
		usleep(20000);
		}
	else 
		{

		ShowRecSerValue(0x82, 0x401C, 1);			//
		usleep(20000);
		}

}


///////////////////////////////////////////////////////////////

/************************************************************/

/*!
 *	\brief	Ñ¡Ôñ¿Ø¼þÍ¨Öª
 *	\details  µ±Ñ¡Ôñ¿Ø¼þ±ä»¯Ê±£¬Ö´ÐÐ´Ëº¯Êý
 *	\param screen_id »­ÃæID
 *	\param control_id ¿Ø¼þID
 *	\param item µ±Ç°Ñ¡Ïî
 */
void NotifySelector(uint16 screen_id, uint16 control_id, uint8 item)
{ //TODO: Ìí¼ÓÓÃ»§´úÂë
}


/*!
											 *	\brief	¶¨Ê±Æ÷³¬Ê±Í¨Öª´¦Àí
											 *	\param screen_id »­ÃæID
											 *	\param control_id ¿Ø¼þID
											 */
void NotifyTimer(uint16 screen_id, uint16 control_id)
{ //TODO: Ìí¼ÓÓÃ»§´úÂë
}


/*!
											 *	\brief	¶ÁÈ¡ÓÃ»§FLASH×´Ì¬·µ»Ø
											 *	\param status 0Ê§°Ü£¬1³É¹¦
											 *	\param _data ·µ»ØÊý¾Ý
											 *	\param length Êý¾Ý³¤¶È
											 */
void NotifyReadFlash(uint8 status, uint8 * _data, uint16 length)
{ //TODO: Ìí¼ÓÓÃ»§´úÂë
}


/*!
											 *	\brief	Ð´ÓÃ»§FLASH×´Ì¬·µ»Ø
											 *	\param status 0Ê§°Ü£¬1³É¹¦
											 */
void NotifyWriteFlash(uint8 status)
{ //TODO: Ìí¼ÓÓÃ»§´úÂë

}


/*!
											 *	\brief	¶ÁÈ¡RTCÊ±¼ä£¬×¢Òâ·µ»ØµÄÊÇBCDÂë
											 *	\param year Äê£¨BCD£©
											 *	\param month ÔÂ£¨BCD£©
											 *	\param week ÐÇÆÚ£¨BCD£©
											 *	\param day ÈÕ£¨BCD£©
											 *	\param hour Ê±£¨BCD£©
											 *	\param minute ·Ö£¨BCD£©
											 *	\param second Ãë£¨BCD£©
											 */
void NotifyReadRTC(uint8 year, uint8 month, uint8 week, uint8 day, uint8 hour, uint8 minute, uint8 second)
{

}


