#include "BS67F350.h"
#include "eeprom.h"


void EEPROM_ByteWrite(unsigned char ADDR,unsigned char byte)
{
	_eea = ADDR;            //地址  BS67f350 的EEPROM地址从0x01-0x7f 共128字节
	_eed = byte;            //数据
	
	_mp1l = 0x40; 			    //EEPROM的物理起始地址
	_mp1h = 0x01;
	
	_emi=0;
	
	_iar1|=0x08;				//写使能  间接寻址EEC寄存器
	_iar1|=0x04;				//开始写入 间接寻址EEC寄存器
	_emi=1;
	
	while((_iar1&0x04) !=0);//等待写入结束 间接寻址EEC寄存器
	_iar1 = 0;
	_mp1h = 0;
		
}

unsigned char EEPROM_ByteRead(volatile unsigned char Addr)
{
	unsigned char byte;
	
	//_emi=0;
	_eea=Addr;              //要读取的地址
	_mp1l = 0x40;           //EEPROM的物理起始地址
	_mp1h = 0x01;
	_iar1|=0x02;			    //读使能       间接寻址EEC寄存器
	_iar1|=0x01;				//开始读取     间接寻址EEC寄存器
	while((_iar1&0x01) !=0);//等待读取结束 间接寻址EEC寄存器

	_iar1 = 0;
	_mp1h = 0;
	byte=_eed;
   return(byte);
}