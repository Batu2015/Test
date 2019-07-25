#ifndef __EEPROM__H_
#define __EEPROM__H_

void EEPROM_ByteWrite(unsigned char ADDR,unsigned char byte);
unsigned char EEPROM_ByteRead(volatile unsigned char Addr);


#endif