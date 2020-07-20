#ifndef __NTC__H__
#define __NTC__H__

#define u8 unsigned char
#define u16 unsigned int


void ntc_init();
u16 getad(u8 ch);
u8 GetTemp();
u16 getad(u8 ch);
	unsigned int Get_Temp(void);
	unsigned int GetAdToTempature();//读AD值转换成温度
unsigned int get_adc_value(void);
#endif