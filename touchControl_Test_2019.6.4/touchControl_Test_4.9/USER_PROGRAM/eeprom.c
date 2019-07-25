#include "BS67F350.h"
#include "eeprom.h"


void EEPROM_ByteWrite(unsigned char ADDR,unsigned char byte)
{
	_eea = ADDR;            //��ַ  BS67f350 ��EEPROM��ַ��0x01-0x7f ��128�ֽ�
	_eed = byte;            //����
	
	_mp1l = 0x40; 			    //EEPROM��������ʼ��ַ
	_mp1h = 0x01;
	
	_emi=0;
	
	_iar1|=0x08;				//дʹ��  ���ѰַEEC�Ĵ���
	_iar1|=0x04;				//��ʼд�� ���ѰַEEC�Ĵ���
	_emi=1;
	
	while((_iar1&0x04) !=0);//�ȴ�д����� ���ѰַEEC�Ĵ���
	_iar1 = 0;
	_mp1h = 0;
		
}

unsigned char EEPROM_ByteRead(volatile unsigned char Addr)
{
	unsigned char byte;
	
	//_emi=0;
	_eea=Addr;              //Ҫ��ȡ�ĵ�ַ
	_mp1l = 0x40;           //EEPROM��������ʼ��ַ
	_mp1h = 0x01;
	_iar1|=0x02;			    //��ʹ��       ���ѰַEEC�Ĵ���
	_iar1|=0x01;				//��ʼ��ȡ     ���ѰַEEC�Ĵ���
	while((_iar1&0x01) !=0);//�ȴ���ȡ���� ���ѰַEEC�Ĵ���

	_iar1 = 0;
	_mp1h = 0;
	byte=_eed;
   return(byte);
}