#ifndef __NTC__H__
#define __NTC__H__

#define u8 unsigned char
#define u16 unsigned int


void ntc_init();
u16 getad(u8 ch);

u8 GetTemp(u8 channel);
	unsigned int GetAdToTempature();//��ADֵת�����¶�
//unsigned int get_adc_value(u8 channel);
#endif