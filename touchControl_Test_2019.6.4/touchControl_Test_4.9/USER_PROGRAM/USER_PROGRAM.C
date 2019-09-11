#include "USER_PROGRAM.H"  
   
#include "ntc.h"
#include "ds1302.h"
#include "stdlib.h"
#include "tm1638.h"
#include "eeprom.h"           

#define u8 unsigned char
#define u16 unsigned int



//可控硅控制引脚
#define SCR_CONTROL	_pa1
//过零检测指示灯
#define LED_detection  _pa5
#define bps9600   12  //sys=8m

typedef struct {
	 unsigned char b0 : 1;
	 unsigned char b1 : 1;
	 unsigned char b2 : 1;
	 unsigned char b3 : 1;
	 unsigned char b4 : 1;
	 unsigned char b5 : 1;
	 unsigned char b6 : 1;
	 unsigned char b7 : 1;
}BITS;

typedef union{
	BITS bits;
	unsigned char data;	
}mybit;                

typedef struct {
	unsigned char enable;
	unsigned char start_time;
	unsigned char end_time;
	unsigned char set_temp;	
}schedule_t;

schedule_t set_week_schedule[7][4] = {0};
//智能模式下设置4组时间
//typedef struct {
//	schedule_t 	time_interval[4];//每天（24小时）可以设置4个时间段	
//}week_schedule_t;
//
//week_schedule_t set_week_schedule[7];
unsigned char adjust_week_index;//星期的调节索引
unsigned char adjust_time_intercal_index;//时间段调节的索引 默认4个时间段，不论到不到24点，都直接跳转到后面一天。
unsigned char set_week_schedule_flag = 0;//周计划


/******************************************************
1.500ms闪烁
2.2分钟设置灯的亮度为0x88
3.5分钟后设置为只显示实时温度值
4.按键扫描上按短按
5.密码锁定，十六进制四位密码，默认初始密码：0000
*************************************************************/
bit start_system = 0;

volatile bit key_lock_flag = 0;//按键长按功能启动后，暂时屏蔽掉长按功能
bit key_confirm_flag = 0;//确认按键
static volatile int contirm_delay = 10000;//20S后无操作，自动确认返回
static volatile bit confirm_lock_key_flag = 0;//按下第一个按键，熄灭led后标志位

volatile unsigned char display_numer[16]={0};
const unsigned char table[] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f,0x80};//0-9
uchar get_time[7] = {0x00,0x00,0x01,0x01,0x01,0x01,0x19};//初始化的时间    //秒 分 时 日 月 周 年 

volatile int delay_num;   //(5ms == 2500 ) 4.9ms 4.8ms....

const unsigned short TimeValueCount[] =
{//3500,3450,3400,3350,3300,3250,3200,3150,3100,3050,
3000,2950,2900,2850,2800,2750,2700,2650,2600,2550,2500,
2450,2400,2350,2300,2250,2200,2150,2100,2050,
2000,1950,1900,1850,1800,1750,1700,1650,1600,
1550,1500,1450,1400,1350,1300,1250,1200,1150,
1100,1050,1000,950,900,850,800,750,700,650,600,
550,500,450,400,350,300,250,200,150,100
};

									
volatile int ctm0_count = 0;//定时计数器
volatile unsigned int stm_flag =0;
volatile unsigned int stm_count = 0;

//实时时钟
volatile char seg_hour = 0;
volatile char seg_second = 0;
volatile char seg_minute = 0;
volatile char seg_week = 1;
volatile char get_new_hour_range = 0;
volatile char get_new_minute_range = 0;


//温度
volatile unsigned char set_tempture_value = 5;
volatile unsigned char set_tempture_max_value = 85;
volatile unsigned char start_tempture = 1;
volatile unsigned char stop_tempture = 1;
volatile unsigned char current_tempture = 20;
//模式选择
volatile bit hengwen_flag = 0;
volatile bit zhineng_flag = 0;

volatile char int0_count = 100;//外部中断计数器
volatile char control_delay = 2;//修改电压变化的时间间隔 默认5S

volatile bit int0_flag = 0;
static char model_index = 1;
static char model_key_flag = 1;
static char model_lock_flag = 1;

//char up_key_flag = 1;
//char down_key_flag = 0;
unsigned char key_add_flag = 0;
unsigned char key_sub_flag = 0;
unsigned char check_long_key_flag = 0;


/*
	10S后无按键操作后，led亮度调到最低，并且只显示当前温度。
	5S后熄灭所有的灯。
*/

volatile char ctm_500ms_flag = 0;

//长按键短按键
volatile bit B_2ms 	= 0;
//volatile bit B_1ms 	= 0;
volatile bit short_key_flag 	= 0;
volatile bit long_key_flag  	= 0;
volatile unsigned int key_hold_ms = 0;


volatile bit short_startup_key_flag = 0;//长按开机键标识
volatile char long_startup_key_flag  = 0;//短按开机键标识
volatile unsigned int startup_key_hold_ms = 0;//保持时间
volatile unsigned int down_key_hold_ms = 0;//保持时间
volatile unsigned int up_key_hold_ms = 0;//保持时间

//调整时间时光标的位置
unsigned char adjust_time_index = 1;
unsigned char new_sec = 0,last_sec =0;


//设置后台功能
//unsigned char set_backstage_num  =0; //
bit set_backstage_flag  =0; //

bit set_display_num_flag = 0;//设置温度栏显示序号
bit backstage_add_flag = 0;
bit backstage_sub_flag = 0;
unsigned int set_Serial_number = 0;

//密码锁定 4位，十六进制
unsigned char coded_lock[4] = {0};

//
void delay_ms(u16 ms)
{
	while(ms--)
	{
		GCC_DELAY(2000);//编译器自带延时指定个周期，在主频8Mhz下，一个指令周期为0.5us	
		GCC_CLRWDT();
	}
}

void delay_50us()
{

	//while(us--)
	{
		GCC_DELAY(100);//编译器自带延时指定个周期，在主频8Mhz下，一个指令周期为0.5us	
		GCC_CLRWDT();
	}	
}

/*
点亮任何位置的led
A-G ==> 1-8
index==表示第几个数码管
number==表示数码管的某一位
enable==表示点亮或熄灭
*/
void display_position_led(int index,char number,char enable)
{
	char i;
	mybit temp1,temp2,temp4,temp6,temp8,temp10,temp12;
//	temp1.data = display_numer[1];  //GR1
	temp2.data = display_numer[2];	//GR2
//	temp4.data = display_numer[4];	//GR2
	
	temp6.data = display_numer[6];
	temp8.data = display_numer[8];	
	temp10.data = display_numer[10];
	temp12.data = display_numer[12];	
	
			
	switch(index)
	{
		case 1:	//seg10 第十个数码管
			for(i = 1;i<=0x0f;i=i+2)
			{
				display_numer[i] &= 0xfd;//seg10 b1位		
			}
			break;		
		case 2:	//seg10 第2个和第三个数码管
			for(i = 1;i<=0x0f;i=i+2)
			{
				display_numer[i] &= 0xfe;//seg9 b0位		
			}
			for(i = 0;i<0x0f;i=i+2)
			{
				display_numer[i] &= 0x7f;//seg8 b7位		
			}
			break;		
		case 3:	//seg10 第4个和第5个数码管
			for(i = 0;i<0x0f;i=i+2)
			{
				display_numer[i] &= 0x9f;//seg7 B5 seg6  b6位		
			}
			break;	
					
		case 10:	//seg1 第十个数码管
		
			if(number == 6)	temp6.bits.b0 = enable;
			if(number == 7)	temp2.bits.b0 = enable;
			if(number == 8)	temp8.bits.b0 = enable;
			if(number == 10)temp10.bits.b0 = enable;
			if(number == 12)temp12.bits.b0 = enable;
			
			break;	
		case 9:	
			temp2.bits.b0 = 1;
			break;
	}
	display_numer[2] = temp2.data;	
	display_numer[6] = temp6.data;	
	display_numer[8] = temp8.data;	
	display_numer[10] = temp10.data;	
	display_numer[12] = temp12.data;	
		
}
/**************************************************
指定位置显示小数点
add:1-10
enable_decimal: 1-灯亮，0-灯灭
*************************************************/
void display_decimal(int index,char enable_decimal)
{
	
	mybit temp1,temp2;
	temp1.data = display_numer[0];
	temp2.data = display_numer[1];	
		
	switch(index)
	{
	
		case 1:	
			temp2.bits.b1 = enable_decimal;
			break;	
		case 2:	
			temp2.bits.b0 = enable_decimal;
			break;
			
		case 3:	
			temp1.bits.b7 = enable_decimal;
			break;
		case 4:	
			temp1.bits.b6 = enable_decimal;
			break;
		case 5:	
			temp1.bits.b5 = enable_decimal;
			break;	
		case 6:	
			temp1.bits.b4 = enable_decimal;
			break;
		case 7:	
			temp1.bits.b3 = enable_decimal;
			break;
		case 8:	
			temp1.bits.b2 = enable_decimal;
			break;
		case 9:	
			temp1.bits.b1 = enable_decimal;
			break;
		case 10:	
			temp1.bits.b0 = enable_decimal;
			break;

	}	
	
		display_numer[0] = temp1.data;	
		display_numer[1] = temp2.data;	
}

//亮度调节
/*
	亮度等级：1-8数字越大led越亮
	TM1638 Led亮度 (0x88-0x8f)8级亮度可调	
*/
void led_light_level(char level)//
{
	if(level > 0 && level < 9)
	{
		Write_COM(0x87+level);	
	}	
}

/***************************************
led全灭
index:1-10

**************************************/
void display_off_all_led( )
{
	char i;
	for(i= 0;i<= 0x0f; i++)
	{
		display_numer[i] = 0x00;	
	}		
//	Write_COM(0x80);
}

/*
显示指定位置的数字
num:0-9
add:1-10
*/
void display_num(char addr,u8 num)
{
	unsigned char tempp = table[num];
	mybit temp[16];
	int i  = 0;
	for(i =0 ;i < 16;i++)
	{
		temp[i].data = display_numer[i];	
	}
			
	switch(addr)
	{
		case 1:{
			for(i = 3;i<16;i = i+2)
			{
				if(tempp & 0x40){
				 temp[i].bits.b1 = 1;	
				}
				else{
				 temp[i].bits.b1 = 0;	
				}
				tempp<<=1;		
			}		
			break;	
		}
		case 2:{
			for(i = 3;i<16;i = i+2)
			{
				if(tempp & 0x40){
				 temp[i].bits.b0 = 1;	
				}
				else{
				 temp[i].bits.b0 = 0;	
				}
				tempp<<=1;		
			}		
			break;	
		}	
		case 3:{
			for(i = 2;i<16;i = i+2)
			{
				if(tempp & 0x40){
				 temp[i].bits.b7 = 1;	
				}
				else{
				 temp[i].bits.b7 = 0;	
				}
				tempp<<=1;		
			}		
			break;	
		}
		case 4:
		{
			for(i = 2;i<16;i = i+2)
			{
				if(tempp & 0x40){
				 temp[i].bits.b6 = 1;	
				}
				else{
				 temp[i].bits.b6 = 0;	
				}
				tempp<<=1;		
			}		
			break;	
		}
		case 5:
		{
			for(i = 2;i<16;i = i+2)
			{
				if(tempp & 0x40){
				 temp[i].bits.b5 = 1;	
				}
				else{
				 temp[i].bits.b5 = 0;	
				}
				tempp<<=1;		
			}		
			break;	
		}
		case 6:
		{
			for(i = 2;i<16;i = i+2)
			{
				if(tempp & 0x40){
				 temp[i].bits.b4 = 1;	
				}
				else{
				 temp[i].bits.b4 = 0;	
				}
				tempp<<=1;		
			}		
			break;	
		}
		case 7:
		{
			for(i = 2;i<16;i = i+2)
			{
				if(tempp & 0x40){
				 temp[i].bits.b3 = 1;	
				}
				else{
				 temp[i].bits.b3 = 0;	
				}
				tempp<<=1;		
			}		
			break;	
		}
		case 8:
		{
			for(i = 2;i<16;i = i+2)
			{
				if(tempp & 0x40){
				 temp[i].bits.b2 = 1;	
				}
				else{
				 temp[i].bits.b2 = 0;	
				}
				tempp<<=1;		
			}		
			break;	
		}
		case 9:
		{
			for(i = 2;i<16;i = i+2)
			{
				if(tempp & 0x40){
				 temp[i].bits.b1 = 1;	
				}
				else{
				 temp[i].bits.b1 = 0;	
				}
				tempp<<=1;		
			}		
			break;	
		}
		case 10:
		{
			for(i = 2;i<16;i = i+2)
			{
				if(tempp & 0x40){
				 temp[i].bits.b0 = 1;	
				}
				else{
				 temp[i].bits.b0 = 0;	
				}
				tempp<<=1;		
			}		
			break;	
		}
	}
	
	for(i =0 ;i < 16;i++)
	{
		display_numer[i] = temp[i].data;
	}
	
}

/*
*显示时间
*
*
*/
void display_RTC_time()//(int seg_week,unsigned int seg_hour,unsigned int seg_minute)
{
	
	display_num(1,seg_week%10);
	display_num(2,seg_hour/10);
	display_num(3,seg_hour%10);
	display_num(4,seg_minute/10);
	display_num(5,seg_minute%10);	
}

//显示设置温度
void display_set_tempture(int set_temp){
	
	display_num(6,set_temp/10);
	display_num(7,set_temp%10);	
}

//显示NTC实时温度
void display_get_NTC_tempture(int get_temp){
	
	display_num(8,get_temp/10);
	display_num(9,get_temp%10);				
}

//关闭某一个数码管 不包括小数点位
int set_seg_led_off(char addr)
{
	mybit temp[16];
	int i  = 0;
	for(i =0 ;i < 16;i++)
	{
		temp[i].data = display_numer[i];	
	}
	
	switch(addr)
	{
		case 1:	
		for(i = 3;i<16;i = i+2){
		  temp[i].bits.b1 = 0;//seg10		
		}
		break;
		case 2:	
		for(i = 3;i<16;i = i+2){
		  temp[i].bits.b0 = 0;//seg9		
		}
		break;		
		case 3:	
		for(i = 2;i<16;i = i+2){
		  temp[i].bits.b7 = 0;//seg8		
		}
		break;
		case 4:	
		for(i = 2;i<16;i = i+2){
		  temp[i].bits.b6 = 0;//seg7		
		}
		break;	
		case 5:	
		for(i = 2;i<16;i = i+2){
		  temp[i].bits.b5 = 0;//seg6		
		}
		break;
		case 6:	
		for(i = 2;i<16;i = i+2){
		  temp[i].bits.b4 = 0;//seg5		
		}
		break;	
		case 7:	
		for(i = 2;i<16;i = i+2){
		  temp[i].bits.b3 = 0;//seg4		
		}
		break;
		case 8:	
		for(i = 2;i<16;i = i+2){
		  temp[i].bits.b2 = 0;//seg3		
		}
		break;	
		case 9:	
		for(i = 2;i<16;i = i+2){
		  temp[i].bits.b1 = 0;//seg2		
		}
		break;
//		case 10:	
//		for(i = 0;i<16;i = i+2){
//		  temp[i].bits.b0 = 0;//seg1		
//		}
//		break;							
	}
			
	for(i =0 ;i < 16;i++)
	{
		display_numer[i] = temp[i].data;	
	}				
}

//TM1638初始化函数
void init_TM1638(void)
{
	unsigned char i;
	Write_COM(0x88);       //亮度 (0x88-0x8f)8级亮度可调
	Write_COM(0x40);       //采用地址自动加1
	STB_L();	           //
	TM1638_Write(0xc0);    //设置起始地址

	for(i=0;i<16;i++)	   //传送16个字节的数据
		TM1638_Write(0x00);
	STB_H();
}

void ctm_init( void )//2ms
{
	//周期是2ms	
	_ctm0c0 = 0x20;//fsys/16 
	_ctm0c1 = 0xc1;//定时器模式、比较计数器A匹配清0计数器
	
	_ctm0al = 0xE8;//(1000)&0xff;
	_ctm0ah = 0x03;
	
	_mf0e= 1;
	_ctm0ae = 1;
	_ct0on 	= 1;
			
}

//外部中断初始化

void int0_init()
{
		//int0
	_pac6 = 1;				//设置为输入 INIT0
	_papu6 = 1;
	_integ = 0x02;	//下降沿
	_int0e = 1;		//使能外部中断			
}

/*
	可控硅导通角定时器
	
*/
void stm_init()//控制可控硅脉冲信号时间
{	
	//stm
	_stmc1 = 0xc1;//定时器模式、比较计数器A匹配清0计数器
	_stmc0 = 0x20;//时钟=sys/16
	
	_stmal = TimeValueCount[delay_num]&0xff;//t=1/(sys/16)*625 s   =5ms
	_stmah = TimeValueCount[delay_num]>>8;//注意先后顺序，先L 后H
	
//	_stmal = (3000 - delay_num) &0xff;
//	_stmal = (3000 - delay_num) >>8;
	
	_mf1e=1;//多功能中断使能
	_stmae = 1;//比较计数器A匹配中断使能
	//_ston=1;//打开定时器
}

//发送单个字符
void UART_SendChar(u8 data)
{
	if(_txif)						//判断发送数据寄存器为空
	{
		_txr_rxr=data;				//写入TXR--------清标志位TXIF，TXIF=1标志TXR写入TSR,0标志禁止写入TXR中已有数据
		while(!_txif);				//等待数据写入TSR
		while(!_tidle);				//等待数据传输结束
	}
}

//发送字符串
void SendString(u8 *ch)
{
	while(*ch!=0)
	{
		UART_SendChar(*(ch++));	
	}		
}

//串口初始化
void UART_Init()
{
	//_pbs07 = 0;//PB3第二功能RX
	//_pbs06 = 1;
	_pbs05 = 0;//PB2第二功能TX
	_pbs04 = 1;
	
	_pbc2=0;
	_pb2=1;
	
	_pbc3=1;
	_pbpu3=1;
	
	_ucr1=0x80;//8位+无奇偶校验位+1个停止位
	_ucr2=0xc4;
	_brg=bps9600;
}


/*
* 模式选择：恒温、恒温峰谷、智能、智能峰谷、休假
*
*led(1-4)-->恒温、智能、峰谷、休假
*/
void key_model_select(char index)
{
	char model_key_flag = 0;	
		
	mybit temp1,temp2,temp3,temp4;
	temp1.data = display_numer[6];
	temp2.data = display_numer[8];	
	temp3.data = display_numer[10];
	temp4.data = display_numer[12];	
		
	switch(index)
	{
		case 1:	
		{
			temp1.bits.b0 = 0;//E10
			temp2.bits.b0 = 0;//D10
			temp3.bits.b0 = 0;//C10 
			temp4.bits.b0 = 1;//B10
			break;	
		}
		case 2:	
		{
			temp1.bits.b0 = 0;//E10
			temp2.bits.b0 = 1;//D10
			temp3.bits.b0 = 0;//C10 
			temp4.bits.b0 = 1;//B10
			break;
		}
		case 3:	
		{
			temp1.bits.b0 = 0;//E10
			temp2.bits.b0 = 0;//D10
			temp3.bits.b0 = 1;//C10 
			temp4.bits.b0 = 0;//B10
			break;
		}
		case 4:	
		{
			temp1.bits.b0 = 0;//E10
			temp2.bits.b0 = 1;//D10
			temp3.bits.b0 = 1;//C10 
			temp4.bits.b0 = 0;//B10
			break;
		}
		case 5:	
		{
			temp1.bits.b0 = 1;//E10
			temp2.bits.b0 = 0;//D10
			temp3.bits.b0 = 0;//C10 
			temp4.bits.b0 = 0;//B10
			break;
		}
		default:
		{
			temp1.bits.b0 = 0;//E10
			temp2.bits.b0 = 0;//D10
			temp3.bits.b0 = 0;//C10 
			temp4.bits.b0 = 1;//B10	
			break;
		}
	}
	
	display_numer[6] =  temp1.data;
	display_numer[8] =  temp2.data;	
	display_numer[10] = temp3.data;
	display_numer[12] = temp4.data;		
}

//显示刷新函数
void display_update()
{
	char i;
	for(i = 0;i< 16;i++)
	{
		Write_DATA(i,display_numer[i]);//grd1 seg1-seg8	
	}	
}
//lvd低电压检测
/*
	判断vdd电压低于4v时自动切换到低速模式
	
*/
void lvd_init()
{
	_lvdc = 0x07;//低于4V
	_mf3e = 1;
	_lvden = 1;//使能
	
}

/*
	系统时钟初始化 8MHZ
*/
void system_clock_init()
{//8Mhz 使能位
	_hircc = 0x01;	
	_hirc0 = 0;
	_hirc1 = 0;//选择8Mhz
	
	_hircen = 1;//使能
}

/*
	某一位数码管显示
*/
void set_display_time()
{
	
	if(adjust_time_index == 1)//星期
	{
	  //display_num(1,10);	
	  set_seg_led_off(1);
	  			
	}
	else if(adjust_time_index == 2)//小时
	{
		set_seg_led_off(2);
		set_seg_led_off(3);
			
	}
	else if(adjust_time_index == 3)//分钟
	{
		set_seg_led_off(4);
		set_seg_led_off(5);							
	}
	else if(adjust_time_index == 4)//设置温度
	{
		set_seg_led_off(6);
		set_seg_led_off(7);							
	}
	else if(adjust_time_index == 5)//小时+分钟
	{
		set_seg_led_off(2);
		set_seg_led_off(3);
		set_seg_led_off(4);
		set_seg_led_off(5);							
	}	
}

/*
	加一按键
*/
void set_temp_add()
{
	switch(adjust_time_index)
	{
		case 1:
		{
			seg_week++;
			if(seg_week > 7)seg_week =1;
			break;	
		}	
		case 2:
		{
			seg_hour++;
			if(seg_hour > 23)seg_hour = 0;
			break;	
		}	
		case 3:
		{
			seg_minute++;
			if(seg_minute > 59)seg_minute = 0;
			break;	
		}
		case 4:
		{
			set_tempture_value++;
			if(set_tempture_value > set_tempture_max_value)set_tempture_value = set_tempture_max_value;
			break;	
		}
		case 5:
		{		
			seg_minute= seg_minute+30;
			if(seg_minute >= 60)
			{
				seg_minute = 0;
				seg_hour++;
				
			}
			
			if(seg_hour >= 24 && seg_minute == 30)
			{
				if(get_new_minute_range == 30)
				{
					seg_hour = 	get_new_hour_range + 1;
					seg_minute = 0;
				}
				else
				{
					seg_hour = 	get_new_hour_range;
					seg_minute = 30;	
				}
			}
					
			break;	
		}
	}	
}

/*
	减一按键
*/
void set_temp_sub()
{
	switch(adjust_time_index)
	{
		case 1:
		{
			seg_week--;
			if(seg_week < 1)seg_week =7;
			break;	
		}	
		case 2:
		{
			seg_hour--;
			if(seg_hour < 0)seg_hour = 23;
			break;	
		}	
		case 3:
		{
			seg_minute--;
			if(seg_minute < 0)seg_minute = 59;
			break;	
		}
		case 4:
		{
			set_tempture_value--;
			if(set_tempture_value < 5)set_tempture_value = 5;
			break;	
		}
		case 5:
		{			
			seg_minute = seg_minute - 30;
			if(seg_minute < 0)
			{
				seg_minute = 30;
				seg_hour--;
			}
			
			if(get_new_minute_range == 30)
			{
				if(seg_hour <= get_new_hour_range-1 && seg_minute == 30 )
				{
					seg_hour = 24;
					seg_minute=0;		
				}
			}
			else 
			{
				if(seg_hour <= get_new_hour_range && seg_minute == 0 )
				{
					seg_hour = 24;
					seg_minute=0;		
				}	
			}
			
			break;	
		}
	}	
}


/*
	周模式默认设置初始化
	3个时间段
	0：00-8：00  5摄氏度
	8：00-16:00   20摄氏度
	16：00-24:00  5摄氏度
	
*/
void week_schudule_init()
{
	char i,j;
	char table_schedule[3][4]={{1,0,8,5},{1,0,16,20},{1,0,24,6}};
	
	for(i = 0;i<7;i++)
	{	
		for(j = 0;j<4;j++) //3个时间段
		{
			if(	set_week_schedule[i][j].enable != 1)
			{
				set_week_schedule[i][j].enable = table_schedule[j][0];
				set_week_schedule[i][j].start_time = table_schedule[j][1];
				set_week_schedule[i][j].end_time = table_schedule[j][2];
				set_week_schedule[i][j].set_temp = table_schedule[j][3];			
			}
		}
	}	
}

/*
	写入周模式到内部存储器eeprom
*/
void write_eeprom_schedule()
{
	char i,j,z=0;
	for(i = 0;i<7;i++)
	{
		for(j = 0;j<4;j++)
		{	
			EEPROM_ByteWrite(z+1,set_week_schedule[i][j].enable);
			EEPROM_ByteWrite(z+2,set_week_schedule[i][j].start_time); 
			EEPROM_ByteWrite(z+3,set_week_schedule[i][j].end_time); 
			EEPROM_ByteWrite(z+4,set_week_schedule[i][j].set_temp); 
			z=z+4;		
		}	
	}
}

/*
	读取已经存储的周模式
*/
void read_eeprom_schedule()
{
	uchar i,j,z=0;
	for(i = 0;i<7;i++)
	{
		for(j = 0;j<4;j++)
		{
			set_week_schedule[i][j].enable     = EEPROM_ByteRead(z+1);
			set_week_schedule[i][j].start_time = EEPROM_ByteRead(z+2);
			set_week_schedule[i][j].end_time   = EEPROM_ByteRead(z+3);
			set_week_schedule[i][j].set_temp   = EEPROM_ByteRead(z+4);
			z=z+4;		
		}	
	}	
}

/******
模式选择：共五种模式。
1.恒温（峰谷）：按上下键自行调整设定温度；
2.智能（峰谷）：一周分为7天，每天时间分段（最多为6个时间段），每个时间段可以分别设定温度。
3.休假：低温运行模式，不允许设定温度，默认5摄氏度。
*****/
void select_model_()
{

		char i;
		switch(model_index)
		{		
			case 1:{//恒温
				hengwen_flag = 1;
				zhineng_flag = 0;
				adjust_time_index = 4;
				//break;
			}
			case 2:{//恒温峰谷
				hengwen_flag = 1;
				zhineng_flag = 0;

				set_tempture_value = EEPROM_ByteRead(0xff);
				adjust_time_index = 4;
				break;
			}	
			case 3:{//智能
				zhineng_flag = 1;
				hengwen_flag = 0;
				
				//break;
			}
			case 4:{//智能峰谷
				zhineng_flag = 1;
				hengwen_flag = 0;
				
				for(i = 0;i<4;i++)
				{
					if(set_week_schedule[seg_week-1][i].enable == 1)
					{
						set_tempture_value = set_week_schedule[seg_week-1][i].set_temp;		
					}	
				}
				break;
			}
			case 5:{//休假
				zhineng_flag = 0;
				hengwen_flag = 0;	
				set_tempture_value = 5;
				break;
			}
		}	
}

//==============================================
//**********************************************
//主程序函数初始化
//==============================================
void USER_PROGRAM_INITIAL()
{
	 int i = 0;
	 unsigned char temp_vaule;

	GCC_CLRWDT();//关闭看门狗
	system_clock_init();//系统时钟初始化
	
	start_system = 1;
	delay_num = 0;//默认关闭可控硅 按下启动按键后启动定时器
	SCR_CONTROL = 1;
	LED_detection = 0;
	

	
	set_Serial_number = 0;//后台功能序号
	temp_vaule = EEPROM_ByteRead(0x7e);
	if(temp_vaule >= 85 || temp_vaule <= 5)
	{
		set_tempture_max_value = 85;	
	}
	else
	{
		set_tempture_max_value = temp_vaule;//上电初始化最大值	
	}
	
	confirm_lock_key_flag = 0;
	adjust_time_index = 4;
	
	adjust_time_intercal_index = 0;//时间段间隔标志位
	adjust_week_index = 0;
	set_week_schedule_flag = 0;//初始化week模式
					    		
	contirm_delay = 10000;//20S
	ctm0_count = 500;
	
	key_lock_flag = 0;
	
	 seg_week = 1;
	 
	 set_tempture_value =5;
	 start_tempture = set_tempture_value - 2;
	 stop_tempture = set_tempture_value + 1;
	 
	 model_index = 1;//开机默认恒温模式
	 hengwen_flag = 1;
	 
	_pac1 = 0;//控制可控硅引脚
	_pac5 = 0;//过零检测指示灯

   
 	init_ds1302();//DS1302实时时钟初始化
 	init_TM1638();//TM1638初始化
 	led_light_level(1);//led灯初始亮度
 	
    ntcinit();//热敏电阻初始化
    UART_Init();//串口初始化

    ctm_init();//1ms定时时间
    int0_init();//外部中断初始化
    stm_init();//定时器初始化，控制可控硅触发

    _emi = 1;//总中断使能

   //led全亮
   	for(i = 1;i<=10;i++)
	{
		 display_num(i,8); 
		 display_decimal(i,1);	 	
	}
      
    display_update();
     
    delay_ms(500);
    display_off_all_led();
    display_update();
    delay_ms(500);
    
	
	read_eeprom_schedule();
	delay_ms(100);	
	if(set_week_schedule[0][0].set_temp == 0xff)
	{
		week_schudule_init();
		write_eeprom_schedule();
		delay_ms(100);			
	}
		
	//上电后读取密码锁的值 0x00== 不锁定按键，否则需要输入密码
	coded_lock[0] = 0x00;//初始化默认是0x0000
	coded_lock[1] = 0x00;
	coded_lock[2] = 0x00;
	coded_lock[3] = 0x00;
	
	if(EEPROM_ByteRead(0x7a) == 0xff || EEPROM_ByteRead(0x7b) == 0xff || EEPROM_ByteRead(0x7c) == 0xff || EEPROM_ByteRead(0x7d) == 0xff)
	{
		EEPROM_ByteWrite(0x7a,coded_lock[0]);
		EEPROM_ByteWrite(0x7b,coded_lock[1]);
		EEPROM_ByteWrite(0x7c,coded_lock[2]);
		EEPROM_ByteWrite(0x7d,coded_lock[3]);
	}
	
	
	//恒温模式下读取eeprom温度值
	if(EEPROM_ByteRead(0x7f) == 0xff) {
		EEPROM_ByteWrite(0x7f,set_tempture_value);
	}
	else {
		set_tempture_value = EEPROM_ByteRead(0x7f);
	};
	
	current_tempture = 80;
	delay_num = 0;
	SCR_CONTROL = 1;
	_ston = 0;


}

//==============================================
//**********************************************
//主程序
//==============================================
void USER_PROGRAM()
{

	unsigned int temp_value;
	uchar i;
	GCC_CLRWDT();	
	GET_KEY_BITMAP();	
			
			
			/*检测是否有按键按下*/
			if(DATA_BUF[1] != 0x00 || DATA_BUF[2] != 0x00)
			{
				contirm_delay = 10000;//10秒
			}

	  		if(B_2ms == 1)
			{
				B_2ms = 0;	
								
				
				if((DATA_BUF[1] & 0x10) == 0x10)//key13 第一个按键 确认按键 返回 熄灭
				{			
					key_confirm_flag = 1;
					startup_key_hold_ms++;
				}	
				else
				{
					if(startup_key_hold_ms >= 2000)//长触摸5S锁定
		    		{
		    			startup_key_hold_ms = 0;
		    			short_startup_key_flag = 0;
		    			long_startup_key_flag ++;
		    			
		    		}
		    		else if(startup_key_hold_ms > 10 && startup_key_hold_ms < 2000 )//时间大于50小于100表示短触摸
		    		{
		    			startup_key_hold_ms = 0;
		    			short_startup_key_flag = 1;	
		    		}
		    		
		    		if(long_startup_key_flag == 1)//锁定键
		    		{
		    			display_decimal(6,1);		
		    		}
		    		else if(long_startup_key_flag == 2){
		    			
		    			display_decimal(6,0);	
		    			long_startup_key_flag = 0;
		    		}
		    	
		    		if(key_confirm_flag == 1 && short_startup_key_flag == 1 && long_startup_key_flag == 0 && start_system == 0 )//确认按键
					{
						key_confirm_flag = 0;
						short_startup_key_flag = 0;
					
						if(zhineng_flag == 1 && set_week_schedule_flag != 0)
						{
							
							set_week_schedule_flag = 0;
							write_eeprom_schedule();//写入eeprom
							return;	
						}	
							
						if(key_lock_flag == 1)
						{
							key_lock_flag = 0;	
							get_time[5] = Data_ToBCD(seg_week);
							get_time[2] = Data_ToBCD(seg_hour);
							get_time[1] = Data_ToBCD(seg_minute);
							
							Set_RTC(get_time);
							display_RTC_time();	
							select_model_();	
						}
						else if(model_lock_flag == 1)
						{
							model_lock_flag = 0;
						}
						else 
						{
						
							if(confirm_lock_key_flag != 1 )//按下熄灭所有led 
							{   
								confirm_lock_key_flag = 1;
								display_off_all_led();
								
								display_position_led(10,7,1);
								display_update();
								//锁定按键标志
								confirm_lock_key_flag  = 1;	
								
								//By 19/2/24 关闭可控硅
								//set_tempture_value = 5;
								current_tempture = 80;
								delay_num = 0;
								SCR_CONTROL = 1;
								_ston = 0;
								start_system = 1;	//系统启动开机标志位
								
							}
							else
							{
								display_position_led(10,7,0);
								display_decimal(1,1);
								display_decimal(4,1);
								display_decimal(2,1);//摄氏度指示灯
								
								display_decimal(5,1);
								display_decimal(7,1);
								key_model_select(model_index);	
							
								confirm_lock_key_flag = 0;
								
								start_system = 0; //系统启动开机标志位
											
							}
						}	
					}
					
				}	
				
				//By 19/2/24
	    		if(start_system == 1 && short_startup_key_flag == 1 && key_confirm_flag == 1)//上电后第一次按下，开机启动按键，
	    		{
	    			short_startup_key_flag = 0;
	    			key_confirm_flag = 0;
	    			
					start_system = 0;
					delay_num = 1;	
					
					display_decimal(1,1);
					display_decimal(4,1);
					display_decimal(2,1);//摄氏度指示灯
					display_decimal(5,1);
					display_decimal(7,1);
					key_model_select(model_index);	
					if(model_index == 1)set_tempture_value = EEPROM_ByteRead(0x7f);
					
	    		}	
		    		
				if(long_startup_key_flag == 0  && start_system == 0)
				{
					//设置时间
					if((DATA_BUF[2] & 0x01) == 0x01 && confirm_lock_key_flag !=1)//触摸一直按下key_hold_ms自加，用来判断长短按键,设置按键，第五个按键
			    	{
			    		key_hold_ms++;
			    	}
			    	else//触摸松开，用来判断长触摸还是短触摸大幅度发到付
			    	{
						if(key_hold_ms >= 150)//时间大于2000表示长触摸 并且led2开始闪烁
			    		{
			    			key_hold_ms = 0;
			    			short_key_flag = 0;
			    			long_key_flag = 1;
			    		}
			    		else if(key_hold_ms > 10 && key_hold_ms < 150 )//时间大于50小于100表示短触摸
			    		{
			    			key_hold_ms = 0;
			    			short_key_flag = 1;
			    			long_key_flag = 0;	
			    		}
			    	}
				}
				
				if(short_key_flag == 1)
			    {
			    	short_key_flag = 0;
			    	long_key_flag = 0;
			    	if(zhineng_flag == 1 && key_lock_flag == 0)//智能模式下，设置周模式
			    	{
			    					    		
			    		if(set_week_schedule_flag == 0)//智能模式，第一次按下，进入设置模式下 0点不可修改
				    	{
				    		set_week_schedule_flag = 1; //设置周模式标志位
				    		adjust_time_intercal_index = 1;
				    		adjust_time_index = 1;//星期先设置
				    		
				    
				    		seg_hour = 0;
				    		seg_minute = 0;
				    		get_new_hour_range = 0;
				    		get_new_minute_range = 0;
				    		set_tempture_value = set_week_schedule[seg_week -1][0].set_temp;
				    		
				    		set_week_schedule[seg_week - 1][adjust_time_intercal_index-1].enable = 1;
				    		set_week_schedule[seg_week -1][0].start_time = 0;
				    		
				    	}
				    	else 
				    	{
				    		if(seg_hour == 24 || adjust_time_intercal_index > 4 )//第一天设置完毕，切换第二天设置
				    		{
				    			seg_hour = 0;
				    		    seg_minute = 0;
				    		    
				    		    get_new_hour_range = 0;
				    			get_new_minute_range = 0;
				    			
				    		    set_tempture_value = set_week_schedule[seg_week][0].set_temp;
				    		   
				    		
				    			if(adjust_time_intercal_index >4)
					    		{
					    			set_week_schedule[seg_week - 1][adjust_time_intercal_index-1].enable = 1;
					    			set_week_schedule[seg_week - 1][adjust_time_intercal_index-1].end_time = seg_hour;	
					    			
					    		}
					    		else {
					    			set_week_schedule[seg_week - 1][adjust_time_intercal_index-1].enable = 1;
					    			set_week_schedule[seg_week - 1][adjust_time_intercal_index-1].end_time = 24;
					    			set_week_schedule[seg_week - 1][adjust_time_intercal_index].enable = 0;	
					    		}
					    		
				    			seg_week++;
				    			if(seg_week > 7)seg_week = 1;
				    				
					    		set_week_schedule[seg_week - 1][0].start_time = 0;
			
					    		set_week_schedule_flag=1;
					    		adjust_time_intercal_index =1;
					    		adjust_time_index = 1;
				    		}
				    		else 
				    		{
				    			adjust_time_index++;
				    			if(adjust_time_index == 2)adjust_time_index=4;
				    		
			    				if(adjust_time_index >= 6)
			    				{
			    					adjust_time_index = 4;
			    					adjust_time_intercal_index++;
			    					set_week_schedule[seg_week - 1][adjust_time_intercal_index-2].end_time = seg_hour;
			    					set_week_schedule[seg_week - 1][adjust_time_intercal_index-1].enable = 1;
			    					set_week_schedule[seg_week - 1][adjust_time_intercal_index-1].start_time = seg_minute;//set_week_schedule[seg_week - 1][adjust_time_intercal_index-1].end_time;
			    			
			    					get_new_hour_range = seg_hour;
				    				get_new_minute_range = seg_minute;
			    				}
				    			if(adjust_time_index == 5)
				    			{//seg_hour = 24;seg_minute=0;
				    				
				    				set_week_schedule[seg_week - 1][adjust_time_intercal_index-1].set_temp = set_tempture_value;
				    			
				    				if(seg_hour < set_week_schedule[seg_week - 1][adjust_time_intercal_index-1].end_time)
				    				{
				    					seg_hour = 	set_week_schedule[seg_week - 1][adjust_time_intercal_index-1].end_time;	
				    				}
				    				
				    				if(seg_hour == 24){
				    					set_tempture_value = set_week_schedule[seg_week-1][adjust_time_intercal_index-1].set_temp;
				    					seg_minute = 0;
				    				}
				    				else 
				    				{
				    					if(adjust_time_intercal_index == 4)
				    					{
				    						seg_hour =24;
				    						seg_minute = 0;
				    						set_tempture_value = 5;		
				    					}
				    					else {
				    						set_tempture_value = set_week_schedule[seg_week-1][adjust_time_intercal_index].set_temp;
				    						seg_minute = set_week_schedule[seg_week - 1][adjust_time_intercal_index].start_time;		
				    					}
	
				    				}
				    				
				    			}
					    		
				    		}	
				    	    		
				    	}	
			    	}
			    
			     	if(set_week_schedule_flag == 0 && key_lock_flag == 1)
			    	{
			    		display_decimal(7,0);
			    	 	adjust_time_index++;//调整时间闪烁位置
				    	if(adjust_time_index >3)
				    	{
				    		adjust_time_index = 1;
				    	}		
			    	}		
			    }
			    else if(long_key_flag == 1)
			    {
			    
			    	key_lock_flag = 1;
			    	
			    	display_decimal(7,1);
			    	short_key_flag = 0;
			    	long_key_flag = 0;
			    	adjust_time_index = 1;//设置时间星期索引，默认现在星期闪烁
			    	//关闭时基中断
			    	//关闭温度采集
			    	//500ms闪烁
			    //	_tb1on  = 0;
			    //	SCR_CONTROL = 1;//关闭可控硅
			    //	delay_num = 10;
			    }
		    		
			}
		
		if(key_lock_flag == 1 || set_week_schedule_flag != 0)
		{
			
			if(ctm_500ms_flag == 1 ) //500ms闪烁一次 //长按标志位置位后就不关闭led
			{
				set_display_time();			
			}
			else 
			{
				display_RTC_time();
				display_set_tempture(set_tempture_value);
					
			}
			
		}

	  
	  	if(ctm_500ms_flag == 1 && key_lock_flag != 1 && start_system == 0 && set_week_schedule_flag == 0)//RTC时间1s显示一次
	  	{
	  		ctm_500ms_flag =0;
	  		Read_RTC(get_time);	 //读取DS1302当前时间
	  		
	  		seg_week = (get_time[5]>>4)*10+(get_time[5]&0x0f);
	  		seg_hour = (get_time[2]>>4)*10+(get_time[2]&0x0f);
	  		seg_minute = (get_time[1]>>4)*10+(get_time[1]&0x0f);
	  				
	  		new_sec = (get_time[0]>>4)*10 +(get_time[0]&0x0f);//秒闪烁功能
	  		
	  		if((new_sec - last_sec) == 1)
	  		{
	  			display_decimal(3,1);
	  		}
	  		else
	  		{
	  			last_sec = new_sec;	
	  			display_decimal(3,0);	
	  		}	
	  	
	  		display_RTC_time();
	  		display_set_tempture(set_tempture_value);
	  		
	  	}

	    if(int0_flag == 1 && key_lock_flag != 1 && confirm_lock_key_flag != 1 && start_system == 0 && set_week_schedule_flag == 0)
	    {
	    	int0_flag = 0;
			current_tempture = GetTemp();//读取当前NTC电阻的实时温度
			if(zhineng_flag == 1)
			{
				for(i = 0;i<4;i++)
				{
					if(set_week_schedule[seg_week-1][i].enable == 1)
					{
						if((seg_hour > set_week_schedule[seg_week-1][i].start_time) && (seg_hour <= set_week_schedule[seg_week][i].start_time))
						 	set_tempture_value = set_week_schedule[seg_week-1][i].set_temp;		
					}		
			 	}		
			}
		
			start_tempture = set_tempture_value -2;
			stop_tempture = set_tempture_value + 1;
			
			display_get_NTC_tempture(current_tempture);
		  	
	    }
						
		if( long_startup_key_flag == 0 && start_system == 0 && set_week_schedule_flag == 0 && key_lock_flag != 1 )//没有锁定下
		{
			if((DATA_BUF[1] & 0x20) == 0x20 && confirm_lock_key_flag != 1)//key14 第二个按键 模式选择按键
			{
				model_key_flag = 1;
			}	
			else
			{
				if(model_key_flag == 1)
				{
					model_key_flag = 0;
					model_lock_flag = 1;
					model_index++;
					if(model_index > 5)
					{
						model_index = 1;	
					}
					key_model_select(model_index);
					select_model_();	
				}
				
			}
		}
		
		if(long_startup_key_flag == 0 && start_system == 0 && model_index != 5 && confirm_lock_key_flag != 1) //没有锁定下 自加减一
		{

				if((DATA_BUF[1] & 0x40) == 0x40)
				{
					
					up_key_hold_ms++;
						
		    		if(up_key_hold_ms > 10 && up_key_hold_ms < 3000 )//时间大于50小于100表示短触摸
		    		{	
		    			
		    			up_key_hold_ms = 0;
		    			if(hengwen_flag == 1){
		    				set_temp_add();	
		    		
		    			}
		    			else 
		    			{
		    				set_temp_add();	
		    							
		    			}
					
						key_add_flag = 1;
						check_long_key_flag = 1;//判断设置周模式时，长按标志位	
							
		    		}
		    				
				}
				else if (key_add_flag == 1 || (up_key_hold_ms >= 1 && up_key_hold_ms <=10))
				{
						if(hengwen_flag == 1){
		    			
		    				EEPROM_ByteWrite(0x7f,set_tempture_value);	
		    			}
	
				
	    				key_add_flag = 0;
	    				up_key_hold_ms = 0;	
	    				check_long_key_flag = 0;
	    				
				}
			
			
				if((DATA_BUF[1] & 0x80) == 0x80)//key16 down 第四个按键 自减一
				{
				
				down_key_hold_ms++;	
				
				if(down_key_hold_ms > 15 && down_key_hold_ms < 4000 )//时间大于50小于100表示短触摸
				{
					down_key_hold_ms = 0;
				
					if(hengwen_flag == 1){
					 set_temp_sub();
						
					}
					else //if(set_week_schedule_flag != 0)
					{
						set_temp_sub();			
					}
					key_sub_flag = 1;
					check_long_key_flag = 1;					    			
				}	
					
				}	
				else if(key_sub_flag == 1  || (down_key_hold_ms>= 1 && down_key_hold_ms<=15))
				{
					
					if(hengwen_flag == 1){
					
					 EEPROM_ByteWrite(0x7f,set_tempture_value);		
					}
				
					key_sub_flag = 0;
					down_key_hold_ms = 0;
					check_long_key_flag = 0;	
				}	  
		}
				
		if(short_startup_key_flag != 1 && start_system == 1 && key_confirm_flag != 1)//后台管理功能
		{
		
			/*
			项目序号   说明
			 1 -------》 可设置的最高温度
			 2 -------》 温度校准 Temperature Calibration ，用于校正测量温度，范围是测量值+、-9.0℃，步进0.5℃
			 3 -------》 温控容差 
			 4 -------》 连续加热器保护触发时间 0-99小时，默认关闭==0
			 5 
			 6 -------》 恢复出厂
				
			*/
			//刷新led显示

		
			if(((DATA_BUF[1] & 0x40) == 0x40) && ((DATA_BUF[1] & 0x20) == 0x20) && ((DATA_BUF[2] & 0x01) == 0x01))//2 3 5三个按键
			{
				set_backstage_flag = 1;			
			}
			
			if(set_backstage_flag == 1)
			{			
	
						
				//if(set_backstage_flag == 1)
				{
					
					//set_backstage_flag = 0;
					
					if((DATA_BUF[2] & 0x01) == 0x01)//切换下一个管理项目，设置温度栏显示序号
					{
						set_display_num_flag = 1;		
					}
					else 
					{
						if(set_display_num_flag == 1)
						{
							set_display_num_flag = 0;
							set_Serial_number++;
							if(set_Serial_number >4)set_Serial_number=0;
						//	display_set_tempture(set_Serial_number);				
						}	
					}
					
					if((DATA_BUF[1] & 0x40) == 0x40)//加
					{
						backstage_add_flag = 1;
						SendString("11111111");
					}
					else
					{
						if(backstage_add_flag == 1)
						{
							backstage_add_flag = 0;
							
							SendString("test add flag: ");
							UART_SendChar(0x30);
							SendString("\r\n");
						
							switch(set_Serial_number)
							{
								case 1:{
									
									set_tempture_max_value++;
									if(set_tempture_max_value > 85)set_tempture_max_value = 85;	
									EEPROM_ByteWrite(0x7e,set_tempture_max_value);	//0x7e 地址存储 后台 可设置的最大值
									
									SendString("test add flag: ");
									UART_SendChar(set_tempture_max_value);
									SendString("\r\n");
								
									break;			
							    }
								case 2:{
									
									break;	
								}
								case 3:{
									
									break;			
							    }
								case 4:{
									
									break;	
								}					
							}
							
						}	
					}
					
					
					if((DATA_BUF[1] & 0x80) == 0x80)//自减
					{
						backstage_sub_flag = 1;
						SendString("2222");
					}
					else
					{
						if(backstage_sub_flag == 1)
						{
							backstage_sub_flag = 0;
							SendString("33333");
							set_tempture_max_value--;
							if(set_tempture_max_value <5)set_tempture_max_value = 5;
							EEPROM_ByteWrite(0x7e,set_tempture_max_value);	//0x7e 地址存储 后台 可设置的最大值
							//display_get_NTC_tempture(set_tempture_max_value);
						//	display_update();	
								
						}
					}	
				       		
				}
				
				display_set_tempture(set_Serial_number);	//显示后台管理功能项目序号
				switch(set_Serial_number)
				{
					case 1:{
						
						display_get_NTC_tempture(set_tempture_max_value);
						break;			
				    }
					case 2:{
						display_get_NTC_tempture(9);							
						break;	
					}
					case 3:{
						display_get_NTC_tempture(3);
						break;			
				    }	
					case 4:{
						display_get_NTC_tempture(2);
						break;	
					}				
				}		
				display_update();
						
			}
				
		}
	

		if(confirm_lock_key_flag != 1 && start_system == 0)//刷新led显示
		{
			if(hengwen_flag == 1){
				display_set_tempture(set_tempture_value);	
			}
			
			display_update();
		}	  	
		
}

//外部中断0
DEFINE_ISR (INT0, 0x04)
{
	GCC_CLRWDT();
		
	if(delay_num < 59 && delay_num >0)
	{
		_ston = 1;//打开定时器stm0		
	}
	
   	if(int0_count-- ==0)//10ms产生一次中断
	{
		int0_count = 100;
		int0_flag = 1;
		control_delay++;
		LED_detection =~ LED_detection;	
	}
	
	if(control_delay == 2)//2s变化一次电压
	{
		control_delay = 0;
		if(current_tempture <= start_tempture)//加热
		{
			delay_num++;
			
			//delay_num=delay_num+50;
			
			if(delay_num >= 59)
			{
				display_decimal(8,1);
				delay_num = 59;
				SCR_CONTROL = 0;
				_ston = 0;	
			}		
		}
		else if(current_tempture >= stop_tempture)//停止
		{
			//delay_num = delay_num-10;//目的是快速关断可控硅	
			delay_num= delay_num - 30;
			if(delay_num < 0)
			{
				display_decimal(8,0);
				delay_num = 0;
				SCR_CONTROL = 1;
				_ston = 0;			
			}	
		}

	}	
}

//CTM0
DEFINE_ISR(ctm0,0x14)
{
	GCC_CLRWDT();
	if(_ctm0af)//中断标志位需要软件清0,中断周期
	{
		_ctm0af = 0;
		B_2ms=1;//判断长短键
	
		if(check_long_key_flag ==1)
		{
			ctm_500ms_flag = 0;
			ctm0_count = 500;		
		}
		
		if(ctm0_count != 0)
		{
			ctm0_count--;			
			
			if(ctm0_count == 250)//250改成400ms
			{
				ctm_500ms_flag = 1;	
			}
			if(ctm0_count == 0)
			{
				ctm_500ms_flag = 0;
				ctm0_count = 500;
			}
		}		
		
		if(key_lock_flag == 1 || set_week_schedule_flag == 1)
		{
			if(--contirm_delay < 0)//20S	
			{
				key_lock_flag = 0;
				contirm_delay = 10000;		
			}	
		}
	}
}

//stm定时器中断 控制可控硅
DEFINE_ISR(stm0,0x18)
{	
	GCC_CLRWDT();
	if(_stmaf)//中断标志位需要软件清0,中断周期5ms
	{
		_stmaf = 0;
		_ston = 0;//关闭定时器stm0
		
		_stmal = TimeValueCount[delay_num]&0xff;//t=1/(sys/16)*625 s   =5ms
		_stmah = TimeValueCount[delay_num]>>8;//注意先后顺序，先L 后H
				
		
		SCR_CONTROL = 0;  
		GCC_DELAY(500);//编译器自带延时指定个周期，在主频8Mhz下，一个指令周期为0.5us	
		SCR_CONTROL = 1;
		
		stm_count++;
		
		if(stm_flag == 0)
		{
			if(stm_count>=500 )//1S
			{	
				stm_count = 0;
				stm_flag = 1;	
			}			
		}
		else {
			if(stm_count>=500 )//1S
			{	
				stm_count = 0;
				stm_flag = 0;	
			}								
		}	 
	}					
}