#include "BS67F350.h"
#include "tm1638.h"


void TM1638_Write(unsigned char	DATA)//TM1638写数据函数
{
	unsigned char i;
	CLK_H();
	for(i=0;i<8;i++)
	{
		CLK_L();
		if(DATA&0X01)
		{
			DIO_H();
		}
		else
		{
			DIO_L();
		}
		DATA>>=1;
		CLK_H();
	}
}
/*
	TM1638 发送命令字
*/
void Write_COM(unsigned char cmd)		//发送命令字
{
	STB_L();
	TM1638_Write(cmd);
	STB_H();
}
/*
	Tm1638 指定地址写入数据
*/
void Write_DATA(unsigned char add,unsigned char DATA)//指定地址写入数据
{
	
	Write_COM(0x44);//固定地址
	STB_L();
	TM1638_Write(0xc0|add);//设置地址oxC0
	TM1638_Write(DATA);
	STB_H();	
}
