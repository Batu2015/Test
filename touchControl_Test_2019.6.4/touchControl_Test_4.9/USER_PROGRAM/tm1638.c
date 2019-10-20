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

//TM1638��ʼ������
void init_TM1638(void)
{
	unsigned char i;
	Write_COM(0x88);       //���� (0x88-0x8f)8�����ȿɵ�
	Write_COM(0x40);       //���õ�ַ�Զ���1
	STB_L();	           //
	TM1638_Write(0xc0);    //������ʼ��ַ

	for(i=0;i<16;i++)	   //����16���ֽڵ�����
		TM1638_Write(0x00);
	STB_H();
}
