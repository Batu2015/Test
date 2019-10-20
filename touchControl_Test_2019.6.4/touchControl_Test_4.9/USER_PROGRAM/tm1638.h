#ifndef __TM1638__H__
#define __TM1638__H__

//TM1638模块引脚定义
//IO操作函数
#define DIO_IN() 	_pec5 = 1
#define DIO_OUT()	_pec5 = 0
#define READ_DIO()	_pe5

#define SDA_IN()  	_pcc3=1
#define SDA_OUT() 	_pcc3=0

#define SCL_IN()  	_pcc5=1
#define SCL_OUT() 	_pcc5=0

#define STB_H() 	{_pec5 = 0;_pe5 = 1;}
#define STB_L() 	{_pec5 = 0;_pe5 = 0;}

#define CLK_H()		{_pec6 = 0;_pe6 = 1;}
#define CLK_L()		{_pec6 = 0;_pe6 = 0;}

#define DIO_H()		{_pec7 = 0;_pe7 = 1;}
#define DIO_L()   	{_pec7 = 0;_pe7 = 0;}

void TM1638_Write(unsigned char	DATA);//TM1638写数据函数
void Write_DATA(unsigned char add,unsigned char DATA);//指定地址写入数据
void Write_COM(unsigned char cmd);		//发送命令字
void init_TM1638(void);

#endif