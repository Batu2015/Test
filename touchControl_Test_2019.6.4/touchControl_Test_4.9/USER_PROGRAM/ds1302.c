#include "BS67F350.h"
#include "ds1302.h"

/*********************************************************************************
* ����    ������ 
* ���������ܡ��� DS1302�����������ʾʱ�� 			   			            			    
* ������˵������ ʱ��ģ���뵥Ƭ������:SCLK- pd5  I/O-Pd6  RST-Pa7   VCC-VCC  GND-GND 
                 
				 ��־λ flag =0 ��ʾʱ����  flag = 1 ��ʾ������  	   
				 
**********************************************************************************/

#define uchar unsigned char
#define uint unsigned int

/* �������� -----------------------------------------------*/

void Read_RTC(uchar *read_time);	 
uchar Read_Ds1302 ( uchar address );
void Write_Ds1302( uchar address,uchar dat );
void Write_Ds1302_Byte(unsigned  char temp);
void senddata(void);
void init_ds1302(void);

void delay(uint z);

/* �������� -----------------------------------------------*/

#ifdef _new_board_

#define RST_H() 	{_pdc5 = 0; _pd5 = 1;}
#define RST_L() 	{_pdc5 = 0; _pd5 = 0;}

#define SCK_H() 	{_pac7 = 0;_pa7 = 1;}
#define SCK_L() 	{_pac7 = 0;_pa7 = 0;}

#else

#define SCK_H() 	{_pdc5 = 0; _pd5 = 1;}
#define SCK_L() 	{_pdc5 = 0; _pd5 = 0;}

#define RST_H() 	{_pac7 = 0;_pa7 = 1;}
#define RST_L() 	{_pac7 = 0;_pa7 = 0;}

#endif

#define SDA_H() 	{_pdc6 = 0;_pd6 = 1;}
#define SDA_L() 	{_pdc6 = 0;_pd6 = 0;}
#define SDA_IN()	_pdc6 = 1
#define SDA			_pd6



#define UP() {SCK_L();nop();SCK_H();nop();} //������  ,ʹ�ú궨�庯��ʱ���һ���ҷֺ�
#define DOWN() {SCK_H();_nop();SCK_L();_nop();} //�½���



unsigned char m;

//unsigned char flag=0;	// ��ʾģʽ 

//unsigned int count;


//uchar l_tmpdate[7]={0,41,19,12,12,4,18};//���ʱ��������2011-07-14 12:00:00
//uchar l_tmpdisplay[8];
//const uchar write_rtc_address[7]={0x80,0x82,0x84,0x86,0x88,0x8a,0x8c}; //���ʱ�������� ���λ��дλ
//const uchar read_rtc_address[7]={0x81,0x83,0x85,0x87,0x89,0x8b,0x8d};  
const uchar init_time[] = {0x30,0x08,0x22,0x23,0x12,0x07,0x18};//��ʼ����ʱ��    //�� �� ʱ �� �� �� �� 


//const unsigned char fseg[]={0xc0,0xf9,0xa4,0xb0,0x99,0x92,0x82,0xf8,0x80,0x90};
//const unsigned char segbit[]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
//unsigned char  disbuf[8]={0,0,0,0,0,0,0,0};


//====================================================================
//===       ʮ����ת��ΪBCD��        ===//
unsigned char Data_ToBCD(unsigned char tData)
{	
	unsigned char tDataH,tDataL;
	tDataH = (tData/10);
	tDataL = (tData%10);
	tDataH <<= 4;
	tData = (tDataH&0xf0)+tDataL;
	return tData;
}

/*
********************************************************************************
** �������� �� Write_Ds1302_Byte(unsigned  char temp)
** �������� �� дһ���ֽ�
********************************************************************************
*/

void Write_Ds1302_Byte(unsigned  char temp) 
{
	uchar i;
	
	for (i=0;i<8;i++)     	//ѭ��8�� д������
	{ 
		if((temp & 0x01) != 0)
		{
			SDA_H();	
		}
		else {
			SDA_L();	
		}
		//_pb6=temp&0x01;     //ÿ�δ�����ֽ� 
		temp>>=1;  		//����һλ
	 	DOWN();
	}
	SDA_H();//�ͷ�IO����
}

/*
********************************************************************************
** �������� �� Write_Ds1302( uchar address,uchar dat )
** �������� �� д��DS1302
********************************************************************************
*/   

void Write_Ds1302( uchar address,uchar dat )     
{
	RST_L();
	_nop();
	SCK_L();
	_nop();
	RST_H();	
	_nop();                    //����
	Write_Ds1302_Byte(address);	//���͵�ַ
	Write_Ds1302_Byte(dat);		//��������
	RST_L();  //����rts�źţ�����дһ���ֽ�
	_nop();
			            //�ָ�
}

uchar ds1302_readbyte()
{
    uint i;
    uchar data_ = 0;
    uint t = 0x01; 
   
    SDA_IN();   
    for(i=0;i<7;i++){     //c51����֧��ֱ����forѭ������ֱ�Ӷ������

        if(SDA){

            data_ = data_ | t;    //��λ��ǰ����λ��ȡ,�տ�ʼ���ԣ����������������
        }                
        t<<=1;
        DOWN();
    }
     return data_;
}

/*
********************************************************************************
** �������� �� Read_Ds1302 ( uchar address )
** �������� �� ����DS1302����
********************************************************************************
*/

uchar Read_Ds1302 ( uchar address )
{
	uchar temp=0x00;
	RST_L();
	_nop();
	_nop();
	SCK_L();
	_nop();
	_nop();
	RST_H();
	_nop();
	_nop();
	Write_Ds1302_Byte(address);
	SDA_IN();
	temp = ds1302_readbyte();
	
	RST_L();
	_nop();	          	//����ΪDS1302��λ���ȶ�ʱ��
	_nop();
	//RST_L();
	//SCK_L();
	SCK_H();
	_nop();
	_nop();
	SDA_L();
	_nop();
	_nop();
	SDA_H();
	_nop();
	_nop();
	return (temp);			//����
}

/*
********************************************************************************
** �������� �� Read_RTC(void)	
** �������� �� ��ʱ������
********************************************************************************
*/

void Read_RTC(uchar *read_time)	        //��ȡ ����
{
	uchar i;//*p;
	//p=read_rtc_address; 	    //��ַ����
	
	for(i=0;i<7;i++)		    //��7�ζ�ȡ ���ʱ��������
	{
		*read_time++ =Read_Ds1302(0x81|(i<<1));
		//*read_time++ = l_tmpdate[i];
		//p++;
	}
}

/*
********************************************************************************
** �������� �� Set_RTC(void)
** �������� �� �趨ʱ������
********************************************************************************
*/

void Set_RTC(unsigned char *time_data)		    //�趨 ����
{
	uchar i;
	
	Write_Ds1302(0x8E,0X00);//����д������
 	//p=write_rtc_address;	//����ַ
 	
 	if(*time_data != 0)
 	{
 		for(i=0;i<7;i++)		//7��д�� ���ʱ��������
	 	{
		  Write_Ds1302(0x80|(i<<1),*time_data);//(*p,l_tmpdate[i]);
		  time_data++;
	 		  //p++;  
		}		
 	}
 	else
 	{
 		for(i=0;i<7;i++)		//7��д�� ���ʱ��������
	 	{
			  Write_Ds1302(0x80|(i<<1),init_time[i]);//(*p,l_tmpdate[i]);
	 		  //p++;  
		}	
 	}	

	 Write_Ds1302(0x8E,0x80);
}

/*
********************************************************************************
** �������� ��delay(uint z)
** �������� �� ��ʱ����	 ��ʱ0.1ms����λ
********************************************************************************
*/
void delay(uint z)
{
	{	while(z--);
  }
} 

/**********************************************************/
// ��ʼ��DS1302
/**********************************************************/
void init_ds1302()
{
   uchar i;
   _pds14 = 0;
   _pds15 = 0;
   _pds13 = 0;
   _pds12 = 0; 
   
   _pas17 = 0;
   _pas16 = 0;
   
   RST_L();
   SCK_L();
   
   i = Read_Ds1302(0x81);//��ȡ��Ĵ���
   if((i & 0x80) !=0)
   {
   		Write_Ds1302(0x8e,0x00);
   		//Set_RTC();
   }
   Write_Ds1302(0x80,0x00); 
   Write_Ds1302(0x90,0x5c); //��ֹ���
   Write_Ds1302(0x8e,0x80); //д���������֣���ֹд
   
}