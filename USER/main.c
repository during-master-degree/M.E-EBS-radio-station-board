#include "led.h"
#include "delay.h"
#include "key.h"
#include "sys.h"
#include "lcd.h"
#include "usart.h"	 	 
#include "audiosel.h"
#include "rda5820.h"
#include "timer.h"
//#include "usmart.h"

#define FREQUENCY_MIN 7600
#define FREQUENCY_MAX 8800
#define FREQUENCY_POINT (FREQUENCY_MAX-FREQUENCY_MIN)/10+1
#define FRAME_WAKEUP_BROADCAST 120*2/4
#define FRAME_WAKEUP_UNICAST 144*2/4
#define FRAME_WAKEUP_MULTICAST 168*2/4
#define FRAME_CONTROL 84*2/4
#define FRAME_SECURTY 470*2/4

//void RDA5820_Show_Msg(void)
//{
//	u8 rssi;
//	u16 freq;
//	freq=RDA5820_Freq_Get();				//读取设置到的频率值
//	LCD_ShowNum(100,210,freq/100,3,16);		//显示频率整数部分
//	LCD_ShowNum(132,210,(freq%100)/10,1,16);//显示频率小数部分
//	rssi=RDA5820_Rssi_Get();				//得到信号强度
//	LCD_ShowNum(100,230,rssi,2,16);			//显示信号强度
//}

u8 flag_frame_processing=0;//收到的数据帧正在处理标志位。1:处理中;0:空闲;
u8 index_frame_send=0;//串口回复信息帧下标
u8 frame_send_buf[100]={0};//串口回传缓冲区

u16 fm_frame_index_bits=0;//FM广播01序列比特流下标
u8 fm_frame_bits[1100]={1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,0,0,0,1,0,0,1,0};//FM广播帧序列比特流缓冲区
u16 fm_frame_index_byte=0;//FM广播01序列字节流下标
u8 fm_frame_byte[1100/2]={0};//FM广播帧序列字节流缓冲区
unsigned char XOR(unsigned char *BUFF, u16 len);



int main(void)
{	 
	u16 freqset=FREQUENCY_MIN;//接收、发射频点
	u16 send_frequency=FREQUENCY_MIN;//发射频点，只保存发射的频率  
	u16 t=0,j=0;
	signed char i=0;//字节转比特流
	u16 len;
	u16 fm_total_bytes=0;//数据帧总有效字节数量
	u8 flag_byte_ready=0;//字节流已经全部保存到本地buffer标志位
//	u8 bit_sync[14]={1,0,1,0,1,0,1,0,1,0,1,0,1,0};//位同步头
//	u8 frame_sync[11]={1,1,1,0,0,0,1,0,0,1,0};//帧同步头	

	delay_init();	    	 //延时函数初始化	  
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口初始化为9600
//	KEY_Init();
 	LED_Init();			     //LED端口初始化
	TIM3_Int_Init(4999,7199);//10Khz的计数频率，计数到5000为500ms
//	LCD_Init();			 	
// 	usmart_dev.init(72);	//初始化USMART	
// 	Audiosel_Init();
//	Audiosel_Set(AUDIO_RADIO);

	RDA5820_Init(); 	
	RDA5820_Band_Set(1);	//设置频段为0:87~108Mhz;1:76~91Mhz;
	RDA5820_Space_Set(0);	//设置步进为100Khz
	RDA5820_TxPGA_Set(3);	//信号增益设置为3
	RDA5820_TxPAG_Set(63);	//发射功率为最大.	
	RDA5820_TX_Mode();			//发送模式
	RDA5820_Freq_Set(send_frequency);	//设置频率
 	while(1)
	{
		if(flag_byte_ready==1){//数据保存本地成功
			fm_frame_index_bits=25;//FM比特流数组清空
			for(t=0;t<fm_frame_index_byte;t++){
				for (i=3;i>=0;i--)
				{
					fm_frame_bits[fm_frame_index_bits]=(fm_frame_byte[t]&(0x01<<i))?1:0; 
					fm_frame_index_bits++;
				}
			}
			for(t=0;t<fm_frame_index_bits;t++)
				{
	//				PBout(6)=fm_frame_bits[t];//还需要加位同步，帧同步头
					USART_SendData(USART1, fm_frame_bits[t]);//向串口发送数据
					while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
				}
			flag_byte_ready=0;//处理完毕，清零
			fm_frame_index_byte=0;//字节流数组清空
			USART_RX_STA=0;//处理完毕，允许接收下一帧
			USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断
		}	

	   if(USART_RX_STA&0x8000)
		{					   
			len=USART_RX_STA&0x3ff;//得到此次接收到的数据长度

			if(USART_RX_BUF[0]=='$'){
				if(USART_RX_BUF[len-1]==XOR(USART_RX_BUF,len-1)){
					index_frame_send=0;
 /*******************************************子板链接判断帧**********************************************************************************/
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
						 //printf("\r\n子板连接帧\r\n");
						 USART_RX_STA=0;//处理完毕，允许接收下一帧
						 USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断
					}
 /*******************************************频谱扫描帧**********************************************************************************/					
					else if((USART_RX_BUF[1]=='f')&&(USART_RX_BUF[2]=='r')&&(USART_RX_BUF[3]=='e')&&(USART_RX_BUF[4]=='_')){//频谱扫描帧
						RDA5820_RX_Mode();			//接收模式
						freqset=FREQUENCY_MIN;
						RDA5820_Freq_Set(freqset);	//设置频率为最小值					

						frame_send_buf[index_frame_send]='$';
						index_frame_send++;
						frame_send_buf[index_frame_send]='f';
						index_frame_send++;
						frame_send_buf[index_frame_send]='r';
						index_frame_send++;
						frame_send_buf[index_frame_send]='e';
						index_frame_send++;
						frame_send_buf[index_frame_send]='_';
						index_frame_send++;
						frame_send_buf[index_frame_send]='_';
						index_frame_send++;
						frame_send_buf[index_frame_send]=USART_RX_BUF[5];
						index_frame_send++;
						frame_send_buf[index_frame_send]=0;
						index_frame_send+=3;

						for(j=0;j<FREQUENCY_POINT;j++){
							
							RDA5820_Freq_Set(freqset);//设置频率
							delay_ms(90);//等待10ms调频信号稳定
							frame_send_buf[8]=RDA5820_Rssi_Get();//得到信号强度
							frame_send_buf[9]=XOR(frame_send_buf,index_frame_send-1);
							for(t=0;t<index_frame_send;t++)
							{
								USART_SendData(USART1, frame_send_buf[t]);//向串口发送数据
								while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
							}
							delay_ms(10);
							frame_send_buf[7]++;

							if(freqset<FREQUENCY_MAX)freqset+=10;  //频率增加100Khz
							else freqset=FREQUENCY_MIN;		 	//越界处理 
						 }

						 RDA5820_TX_Mode();			//频谱扫描之后切换回发送模式
						 RDA5820_Freq_Set(send_frequency);	//设置频率，换为全局变量
						 delay_ms(20);
						 //printf("\r\n频谱扫描帧\r\n");
						 USART_RX_STA=0;//处理完毕，允许接收下一帧
						 USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断
					}
 /*******************************************控制帧**********************************************************************************/
					else if((USART_RX_BUF[1]=='c')&&(USART_RX_BUF[2]=='o')&&(USART_RX_BUF[3]=='n')&&(USART_RX_BUF[4]=='_')){//控制帧
						frame_send_buf[index_frame_send]='$';
						 index_frame_send++;
						 frame_send_buf[index_frame_send]='c';
						 index_frame_send++;
						 frame_send_buf[index_frame_send]='o';
						 index_frame_send++;
						 frame_send_buf[index_frame_send]='n';
						 index_frame_send++;
						 frame_send_buf[index_frame_send]='_';
						 index_frame_send++;
						 frame_send_buf[index_frame_send]='_';
						 index_frame_send++;
						 frame_send_buf[index_frame_send]=USART_RX_BUF[5];
						 index_frame_send++;
						 frame_send_buf[index_frame_send]=XOR(frame_send_buf,index_frame_send);
						 index_frame_send++;
						 switch(USART_RX_BUF[6]){
						 	case 1:
							break;
							case 2:
								send_frequency=USART_RX_BUF[7]*10;
								RDA5820_Freq_Set(send_frequency);	//设置频率，换为全局变量
							break;
							default:
							break;
						 }
						 for(t=0;t<index_frame_send;t++)
						 {
							USART_SendData(USART1, frame_send_buf[t]);//向串口发送数据
							while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
						 }
						 //printf("\r\n控制帧\r\n");
						 USART_RX_STA=0;//处理完毕，允许接收下一帧
						 USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断
					}
 /*******************************************数据帧**********************************************************************************/					
					else if((USART_RX_BUF[1]=='d')&&(USART_RX_BUF[2]=='a')&&(USART_RX_BUF[3]=='t')&&(USART_RX_BUF[4]=='_')){//数据帧
						frame_send_buf[index_frame_send]='$';
						index_frame_send++;
						frame_send_buf[index_frame_send]='d';
						index_frame_send++;
						frame_send_buf[index_frame_send]='a';
						index_frame_send++;
						frame_send_buf[index_frame_send]='t';
						index_frame_send++;
						frame_send_buf[index_frame_send]='_';
						index_frame_send++;
						frame_send_buf[index_frame_send]='_';
						index_frame_send++;
						frame_send_buf[index_frame_send]=USART_RX_BUF[5];
						index_frame_send++;
						frame_send_buf[index_frame_send]=XOR(frame_send_buf,index_frame_send);
						index_frame_send++;
				   /*******************数据帧处理，1字节转为4bits，开始*********************************/
				   		fm_total_bytes=USART_RX_BUF[6]*256+USART_RX_BUF[7];
					if((fm_total_bytes==FRAME_WAKEUP_BROADCAST )||(fm_total_bytes==FRAME_WAKEUP_UNICAST  )||(fm_total_bytes==FRAME_WAKEUP_MULTICAST  )||(fm_total_bytes==FRAME_CONTROL  )||(fm_total_bytes==FRAME_SECURTY  )){
						for(t=0;t<fm_total_bytes;t++){
							fm_frame_byte[fm_frame_index_byte]=USART_RX_BUF[t+8]-0x30;
							fm_frame_index_byte++;
						}
						flag_byte_ready=1;//数据帧字节流保存到本地，成功
				   /*******************数据帧处理，1字节转为4bits，结束*********************************/		
						for(t=0;t<index_frame_send;t++)
						{
//							USART_SendData(USART1, frame_send_buf[t]);//向串口发送数据
//							while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
						}
					}else{
						flag_byte_ready=0;//数据帧字节流保存到本地，未成功
						fm_frame_index_byte=0;//字节流数组清空
//						printf("\r\nData frame error.\r\n");
						}
					}//else{printf("Frame anomalous!");}
				}//else{printf("Verify bits wrong!");}
		    }//if(USART_RX_BUF[0]=='$')
						
		}//if(USART_RX_STA&0x8000)
			
	}//while(1)
}





unsigned char XOR(unsigned char *BUFF, u16 len)
{
	unsigned char result=0;
	u16 i;
	for(result=BUFF[0],i=1;i<len;i++)
	{
		result ^= BUFF[i];
	}
	return result;
}







