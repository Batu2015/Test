#include "BS67F350.h"
#include "ds1302.h"

/*********************************************************************************
* 【声    明】： 
* 【函数功能】： DS1302串行数码管显示时钟 			   			            			    
* 【接线说明】： 时钟模块与单片机连接:SCLK- pd5  I/O-Pd6  RST-Pa7   VCC-VCC  GND-GND 
                 
				 标志位 flag =0 显示时分秒  flag = 1 显示年月日  	   
				 
**********************************************************************************/

#define uchar unsigned char
#define uint unsigned int

/* 函数申明 -----------------------------------------------*/

void Read_RTC(uchar *read_time);	 
uchar Read_Ds1302 ( uchar address );
void Write_Ds1302( uchar address,uchar dat );
void Write_Ds1302_Byte(unsigned  char temp);
void senddata(void);
void init_ds1302(void);

void delay(uint z);

/* 变量定义 -----------------------------------------------*/

#ifdef _new_board_

#define RST_H() 	{_pdc5 = 0; _pd5 = 1;}
#define RST_L() 	{_pdc5 = 0; _pd5 = 0;}

#define SCK_H() 	{_pac7 = 0;_pa7 = 1;}
#define SCK_L() 	{_pac7 = 0;_pa7 = 0;}

#else

#define SCK_H() 	{_pdc5 = 0; _pd5 = 1;}
#define SCK_L() 	{_pdc5 = 0; _pd5 = 0;}

#define RST_H() 	{_pac7 = 0;_pa7 = 1;}
#define RST_L() 	{_pac7 = 0;_pa7 = 0;}

#endif

#define SDA_H() 	{_pdc6 = 0;_pd6 = 1;}
#define SDA_L() 	{_pdc6 = 0;_pd6 = 0;}
#define SDA_IN()	_pdc6 = 1
#define SDA			_pd6



#define UP() {SCK_L();nop();SCK_H();nop();} //上升沿  ,使用宏定义函数时最后一定家分号
#define DOWN() {SCK_H();_nop();SCK_L();_nop();} //下降沿



unsigned char m;

//unsigned char flag=0;	// 显示模式 

//unsigned int count;


//uchar l_tmpdate[7]={0,41,19,12,12,4,18};//秒分时日月周年2011-07-14 12:00:00
//uchar l_tmpdisplay[8];
//const uchar write_rtc_address[7]={0x80,0x82,0x84,0x86,0x88,0x8a,0x8c}; //秒分时日月周年 最低位读写位
//const uchar read_rtc_address[7]={0x81,0x83,0x85,0x87,0x89,0x8b,0x8d};  
const uchar init_time[] = {0x30,0x08,0x22,0x23,0x12,0x07,0x18};//初始化的时间    //秒 分 时 日 月 周 年 


//const unsigned char fseg[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90};
//const unsigned char segbit[]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
//unsigned char  disbuf[8]={0,0,0,0,0,0,0,0};


//====================================================================
//===       十进制转换为BCD码        ===//
unsigned char Data_ToBCD(unsigned char tData)
{	
	unsigned char tDataH,tDataL;
	tDataH = (tData/10);
	tDataL = (tData%10);
	tDataH <<= 4;
	tData = (tDataH&0xf0)+tDataL;
	return tData;
}

/*
********************************************************************************
** 函数名称 ： Write_Ds1302_Byte(unsigned  char temp)
** 函数功能 ： 写一个字节
********************************************************************************
*/

void Write_Ds1302_Byte(unsigned  char temp) 
{
	uchar i;
	
	for (i=0;i<8;i++)     	//循环8次 写入数据
	{ 
		if((temp & 0x01) != 0)
		{
			SDA_H();	
		}
		else {
			SDA_L();	
		}
		//_pb6=temp&0x01;     //每次传输低字节 
		temp>>=1;  		//右移一位
	 	DOWN();
	}
	SDA_H();//释放IO引脚
}

/*
********************************************************************************
** 函数名称 ： Write_Ds1302( uchar address,uchar dat )
** 函数功能 ： 写入DS1302
********************************************************************************
*/   

void Write_Ds1302( uchar address,uchar dat )     
{
	RST_L();
	_nop();
	SCK_L();
	_nop();
	RST_H();	
	_nop();                    //启动
	Write_Ds1302_Byte(address);	//发送地址
	Write_Ds1302_Byte(dat);		//发送数据
	RST_L();  //拉低rts信号，结束写一个字节
	_nop();
			            //恢复
}

uchar ds1302_readbyte()
{
    uint i;
    uchar data_ = 0;
    uint t = 0x01; 
   
    SDA_IN();   
    for(i=0;i<7;i++){     //c51好像不支持直接在for循环里面直接定义变量

        if(SDA){

            data_ = data_ | t;    //低位在前，逐位读取,刚开始不对，估计是这个的问题
        }                
        t<<=1;
        DOWN();
    }
     return data_;
}

/*
********************************************************************************
** 函数名称 ： Read_Ds1302 ( uchar address )
** 函数功能 ： 读出DS1302数据
********************************************************************************
*/

uchar Read_Ds1302 ( uchar address )
{
	uchar temp=0x00;
	RST_L();
	_nop();
	_nop();
	SCK_L();
	_nop();
	_nop();
	RST_H();
	_nop();
	_nop();
	Write_Ds1302_Byte(address);
	SDA_IN();
	temp = ds1302_readbyte();
	
	RST_L();
	_nop();	          	//以下为DS1302复位的稳定时间
	_nop();
	//RST_L();
	//SCK_L();
	SCK_H();
	_nop();
	_nop();
	SDA_L();
	_nop();
	_nop();
	SDA_H();
	_nop();
	_nop();
	return (temp);			//返回
}

/*
********************************************************************************
** 函数名称 ： Read_RTC(void)	
** 函数功能 ： 读时钟数据
********************************************************************************
*/

void Read_RTC(uchar *read_time)	        //读取 日历
{
	uchar i;//*p;
	//p=read_rtc_address; 	    //地址传递
	
	for(i=0;i<7;i++)		    //分7次读取 秒分时日月周年
	{
		*read_time++ =Read_Ds1302(0x81|(i<<1));
		//*read_time++ = l_tmpdate[i];
		//p++;
	}
}

/*
********************************************************************************
** 函数名称 ： Set_RTC(void)
** 函数功能 ： 设定时钟数据
********************************************************************************
*/

void Set_RTC(unsigned char *time_data)		    //设定 日历
{
	uchar i;
	
	Write_Ds1302(0x8E,0X00);//允许写入数据
 	//p=write_rtc_address;	//传地址
 	
 	if(*time_data != 0)
 	{
 		for(i=0;i<7;i++)		//7次写入 秒分时日月周年
	 	{
		  Write_Ds1302(0x80|(i<<1),*time_data);//(*p,l_tmpdate[i]);
		  time_data++;
	 		  //p++;  
		}		
 	}
 	else
 	{
 		for(i=0;i<7;i++)		//7次写入 秒分时日月周年
	 	{
			  Write_Ds1302(0x80|(i<<1),init_time[i]);//(*p,l_tmpdate[i]);
	 		  //p++;  
		}	
 	}	

	 Write_Ds1302(0x8E,0x80);
}

/*
********************************************************************************
** 函数名称 ：delay(uint z)
** 函数功能 ： 延时函数	 延时0.1ms个单位
********************************************************************************
*/
void delay(uint z)
{
	{	while(z--);
  }
} 

/**********************************************************/
// 初始化DS1302
/**********************************************************/
void init_ds1302()
{
   uchar i;
   _pds14 = 0;
   _pds15 = 0;
   _pds13 = 0;
   _pds12 = 0; 
   
   _pas17 = 0;
   _pas16 = 0;
   
   RST_L();
   SCK_L();
   
   i = Read_Ds1302(0x81);//读取秒寄存器
   if((i & 0x80) !=0)
   {
   		Write_Ds1302(0x8e,0x00);
   		//Set_RTC();
   }
   Write_Ds1302(0x80,0x00); 
   Write_Ds1302(0x90,0x5c); //禁止充电
   Write_Ds1302(0x8e,0x80); //写保护控制字，禁止写
   
}