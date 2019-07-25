#include "BS67F350.h"
#include "tm1638.h"


void TM1638_Write(unsigned char	DATA)//TM1638д���ݺ���
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
	TM1638 ����������
*/
void Write_COM(unsigned char cmd)		//����������
{
	STB_L();
	TM1638_Write(cmd);
	STB_H();
}
/*
	Tm1638 ָ����ַд������
*/
void Write_DATA(unsigned char add,unsigned char DATA)//ָ����ַд������
{
	
	Write_COM(0x44);//�̶���ַ
	STB_L();
	TM1638_Write(0xc0|add);//���õ�ַoxC0
	TM1638_Write(DATA);
	STB_H();	
}
