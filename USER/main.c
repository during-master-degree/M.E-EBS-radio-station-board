#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "lcd.h"
#include "usart.h"	 	 
#include "audiosel.h"
#include "rda5820.h"
#include "usmart.h"

void RDA5820_Show_Msg(void)
{
	u8 rssi;
	u16 freq;
	freq=RDA5820_Freq_Get();				//读取设置到的频率值
	LCD_ShowNum(100,210,freq/100,3,16);		//显示频率整数部分
	LCD_ShowNum(132,210,(freq%100)/10,1,16);//显示频率小数部分
	rssi=RDA5820_Rssi_Get();				//得到信号强度
	LCD_ShowNum(100,230,rssi,2,16);			//显示信号强度
}

u8 flag_frame_processing=0;//收到的数据帧正在处理标志位。1:处理中;0:空闲;
u8 index_frame_send=0;//回复信息帧下标
u8 frame_send_buf[100]={0};//发送缓冲区
unsigned char XOR(unsigned char *BUFF, int len);



int main(void)
{	 
	u8 key,rssi;
	u16 freqset=8700;//默认为87Mhz  
	u8 i=0;
	u8 mode=0;	//接收模式

	u16 t;
	u8 len;	

	delay_init();	    	 //延时函数初始化	  
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为9600
	KEY_Init();
 	LED_Init();			     //LED端口初始化
	LCD_Init();			 	
   	usmart_dev.init(72);	//初始化USMART	
 	Audiosel_Init();
	Audiosel_Set(AUDIO_RADIO);

	RDA5820_Init();	  

 	POINT_COLOR=RED;//设置字体为红色 
	LCD_ShowString(60,50,200,16,16,"WarShip STM32");	
	LCD_ShowString(60,70,200,16,16,"RDA5820 TEST");	
	LCD_ShowString(60,90,200,16,16,"ATOM@ALIENTEK");
	LCD_ShowString(60,110,200,16,16,"2012/9/14");  
	LCD_ShowString(60,130,200,16,16,"KEY0:Freq+ KEY2:Freq-");
	LCD_ShowString(60,150,200,16,16,"KEY1:Auto Search(RX)");
	LCD_ShowString(60,170,200,16,16,"KEY_UP:Mode Set");
	POINT_COLOR=BLUE;
	//显示提示信息
	POINT_COLOR=BLUE;//设置字体为蓝色	  
	LCD_ShowString(60,190,200,16,16,"Mode:FM RX");	 				    
	LCD_ShowString(60,210,200,16,16,"Freq: 93.6Mhz");	 				    
	LCD_ShowString(60,230,200,16,16,"Rssi:");	 				    
 	
	RDA5820_Band_Set(0);	//设置频段为87~108Mhz
	RDA5820_Space_Set(0);	//设置步进为100Khz
	RDA5820_TxPGA_Set(3);	//信号增益设置为3
	RDA5820_TxPAG_Set(63);	//发射功率为最大.
	RDA5820_RX_Mode();		//设置为接收模式  
	
	freqset=9360;				//默认为93.6Mhz
	RDA5820_Freq_Set(freqset);	//设置频率
 	while(1)
	{	
//		key=KEY_Scan(0);//不支持连按
//		switch(key)
//		{
//			case 0://无任何按键按下
//				break;
//			case KEY_UP://切换模式
//				mode=!mode;
//				if(mode)
//				{
//					Audiosel_Set(AUDIO_PWM);	//设置到PWM 音频通道
//					RDA5820_TX_Mode();			//发送模式
//					RDA5820_Freq_Set(freqset);	//设置频率
//					LCD_ShowString(100,190,200,16,16,"FM TX");	 				    
//				}else 
//				{
//						Audiosel_Set(AUDIO_RADIO);	//设置到收音机声道
//						RDA5820_RX_Mode();			//接收模式
//					RDA5820_Freq_Set(freqset);	//设置频率
//					LCD_ShowString(100,190,200,16,16,"FM RX");	 				    
//				}
//				break;
//			case KEY_DOWN://自动搜索下一个电台.
//				if(mode==0)//仅在接收模式有效
//				{
//  					while(1)
//					{
//						if(freqset<10800)freqset+=10;   //频率增加100Khz
//					 	else freqset=8700;				//回到起点
//						RDA5820_Freq_Set(freqset);		//设置频率
//						delay_ms(10);					//等待调频信号稳定
//						if(RDA5820_RD_Reg(0X0B)&(1<<8))//是一个有效电台. 
//						{
//  							RDA5820_Show_Msg();			//显示信息
//							break;		 
//						}		    		
// 						RDA5820_Show_Msg();			//显示信息
//						//在搜台期间有按键按下,则跳出搜台.
//						key=KEY_Scan(0);//不支持连按
//						if(key)break;
//					}
//				}
//				break;
//			case KEY_LEFT://频率减
//				if(freqset>8700)freqset-=10;   //频率减少100Khz
//				else freqset=10800;			//越界处理 
//				RDA5820_Freq_Set(freqset);	//设置频率
//				RDA5820_Show_Msg();//显示信息
//				break;
//			case KEY_RIGHT://频率增
//				if(freqset<10800)freqset+=10;  //频率增加100Khz
//				else freqset=8700;		 	//越界处理 
//				RDA5820_Freq_Set(freqset);	//设置频率
//				RDA5820_Show_Msg();//显示信息
//				break;
//		}	 
//		i++;
//		delay_ms(10);	  
//		if(i==200)//每两秒左右检测一次信号强度等信息.
//		{
//			i=0;
//			rssi=RDA5820_Rssi_Get();				//得到信号强度
//			LCD_ShowNum(100,230,rssi,2,16);			//显示信号强度
//		}

//	   printf("%d ,",USART_RX_STA);
	   if(USART_RX_STA&0x8000)
		{					   
			len=USART_RX_STA&0x3ff;//得到此次接收到的数据长度
			for(t=0;t<index_frame_send;t++)
						 {
							USART_SendData(USART1, frame_send_buf[t]);//向串口发送数据
							while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
						 }

			if(USART_RX_BUF[0]=='$'){
				if(USART_RX_BUF[len-1]==XOR(USART_RX_BUF,len)){
					index_frame_send=0;
					if((USART_RX_BUF[1]=='r')&&(USART_RX_BUF[2]=='d')&&(USART_RX_BUF[3]=='y')&&(USART_RX_BUF[4]=='_')){//连接帧
						 frame_send_buf[index_frame_send]='$';
						 index_frame_send++;
						 frame_send_buf[index_frame_send]='r';
						 index_frame_send++;
						 frame_send_buf[index_frame_send]='d';
						 index_frame_send++;
						 frame_send_buf[index_frame_send]='y';
						 index_frame_send++;
						 frame_send_buf[index_frame_send]='_';
						 index_frame_send++;
						 frame_send_buf[index_frame_send]='_';
						 index_frame_send++;
						 frame_send_buf[index_frame_send]=USART_RX_BUF[5];
						 index_frame_send++;
						 frame_send_buf[index_frame_send]=XOR(frame_send_buf,index_frame_send);
						 index_frame_send++;
						 for(t=0;t<index_frame_send;t++)
						 {
							USART_SendData(USART1, frame_send_buf[t]);//向串口发送数据
							while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
						 }
						 printf("ok！");
	
					}else if((USART_RX_BUF[1]=='f')&&(USART_RX_BUF[2]=='r')&&(USART_RX_BUF[3]=='e')&&(USART_RX_BUF[4]=='_')){//频谱扫描帧
					
					}else if((USART_RX_BUF[1]=='c')&&(USART_RX_BUF[2]=='o')&&(USART_RX_BUF[3]=='n')&&(USART_RX_BUF[4]=='_')){//控制帧
					
					}else if((USART_RX_BUF[1]=='d')&&(USART_RX_BUF[2]=='a')&&(USART_RX_BUF[3]=='t')&&(USART_RX_BUF[4]=='_')){//数据帧
					
					}else{printf("帧异常！");}
				}else{printf("校验位错误！");}
		    }
//			for(t=0;t<len;t++)
//			{
//				USART_SendData(USART1, USART_RX_BUF[t]);//向串口1发送数据
//				while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
//			}

			USART_RX_STA=0;
		}
	
	}
}





unsigned char XOR(unsigned char *BUFF, int len)
{
	unsigned char result=0;
	int i;
	for(result=BUFF[0],i=1;i<len;i++)
	{
		result ^= BUFF[i];
	}
	return result;
}







