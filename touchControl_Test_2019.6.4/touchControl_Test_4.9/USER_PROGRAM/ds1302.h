#ifndef __DS1302__H__
#define __DS1302__H__

#define uchar unsigned char
#define uint unsigned int


/* º¯ÊıÉêÃ÷ -----------------------------------------------*/
void Set_RTC(unsigned char *time_data);

//void Read_RTC(void);
uchar Read_Ds1302 ( uchar address );
void Write_Ds1302( uchar address,uchar dat );
void Write_Ds1302_Byte(unsigned  char temp);
void Read_RTC(uchar *read_time);	 
void init_ds1302(void);

unsigned char Data_ToBCD(unsigned char tData);

#endif