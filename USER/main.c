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
#define FREQUENCY_MAX 10100
#define FREQUENCY_POINT (FREQUENCY_MAX-FREQUENCY_MIN)/10+1
#define FRAME_WAKEUP_BROADCAST 120*2/4
#define FRAME_WAKEUP_UNICAST 144*2/4
#define FRAME_WAKEUP_MULTICAST 168*2/4
#define FRAME_CONTROL 84*2/4
#define FRAME_SECURTY 470*2/4

u8 flag_frame_processing=0;//收到的数据帧正在处理标志位。1:处理中;0:空闲;
u8 index_frame_send=0;//串口回复信息帧下标
u8 frame_send_buf[100]={0};//串口回传缓冲区

u16 fm_frame_index_bits=0;//FM广播01序列比特流下标
u8 fm_frame_bits[1100]={1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,0,0,0,1,0,0,1,0};//FM广播帧序列比特流缓冲区
u16 fm_frame_index_byte=0;//FM广播01序列字节流下标
u8 fm_frame_byte[1100/2]={0};//FM广播帧序列字节流缓冲区

void frame_resent(void);
u8 main_busy=0;//主循环中正在执行
u8 flag_byte_ready=0;//字节流已经全部保存到本地buffer标志位

u8 temp_frame_byte[1100/2]={0};//测试数组
u16 temp_frame_byte_index=0;//测试数组


int main(void)
{	 
	u16 freqset=FREQUENCY_MIN;//接收、发射频点
	u16 send_frequency=FREQUENCY_MIN;//发射频点，只保存发射的频率  
	u16 t=0,j=0;
	signed char i=0;//字节转比特流
	u16 len;
	u16 fm_total_bytes=0;//数据帧总有效字节数量

//	u16 times=0;

//	u8 bit_sync[14]={1,0,1,0,1,0,1,0,1,0,1,0,1,0};//位同步头
//	u8 frame_sync[11]={1,1,1,0,0,0,1,0,0,1,0};//帧同步头	

	delay_init();	    	 //延时函数初始化	  
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口1初始化为9600
	uart2_init(115200);	 //串口2初始化为4800
//	KEY_Init();
 	LED_Init();			     //LED端口初始化
	TIM5_Int_Init(9999,7199);//10Khz的计数频率，计数到8000为800ms
	TIM3_Int_Init(9,7199);//1Khz的计数频率
	TIM7_Int_Init(4,1199);//6K周期方波
	TIM6_Int_Init(4,719);//10k周期方波
	tim3_pin_init(); 
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
		if(USART2_RX_STA&0x80)
		{					   
			len=USART2_RX_STA&0x3ff;//得到此次接收到的数据长度
			for(t=0;t<len;t++)
			{
				USART_SendData(USART2, USART2_RX_BUF[t]);  //向串口2发送数据
				while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);//等待发送结束
			}
			USART2_RX_STA=0;
		}



		if(flag_byte_ready==1){//数据保存本地成功
			fm_frame_index_bits=25;//FM比特流数组重置
			for(t=0;t<fm_frame_index_byte;t++){
				for (i=3;i>=0;i--)
				{
					fm_frame_bits[fm_frame_index_bits]=(fm_frame_byte[t]&(0x01<<i))?1:0; 
					fm_frame_index_bits++;
				}
			}
			TIM_Cmd(TIM3, ENABLE); //使能TIMx
			TIM_Cmd(TIM6, ENABLE); //使能TIMx
			TIM_Cmd(TIM7, ENABLE); //使能TIMx
			flag_frame_processing=1;
//			for (temp_frame_byte_index=0;temp_frame_byte_index<fm_frame_index_bits/4;temp_frame_byte_index++)
//			{
//				temp_frame_byte[temp_frame_byte_index]=fm_frame_bits[temp_frame_byte_index*4]*8+fm_frame_bits[temp_frame_byte_index*4+1]*4+fm_frame_bits[temp_frame_byte_index*4+2]*2+fm_frame_bits[temp_frame_byte_index*4+3]*1+0x30;//ASCII 0码对应十进制是0x30
//			}

//			for(t=0;t<fm_frame_index_bits;t++)
//			{
//				PBout(6)=fm_frame_bits[t];//还需要加位同步，帧同步头
//				delay_ms(1);
//			//	USART_SendData(USART1, temp_frame_byte[t]);//向串口发送数据
//			//	while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
//			}
			flag_byte_ready=0;//处理完毕，清零
			fm_frame_index_byte=0;//字节流数组清空
			USART_RX_STA=0;//处理完毕，允许接收下一帧
		}	

	   if((USART_RX_STA&0x8000)&&(flag_byte_ready==0))
		{					   
			len=USART_RX_STA&0x3ff;//得到此次接收到的数据长度

			if(USART_RX_BUF[0]=='$'){
				if(USART_RX_BUF[len-1]==XOR(USART_RX_BUF,len-1)){
					index_frame_send=0;
 /*******************************************子板链接判断帧**********************************************************************************/
					if((USART_RX_BUF[1]=='r')&&(USART_RX_BUF[2]=='d')&&(USART_RX_BUF[3]=='y')&&(USART_RX_BUF[4]=='_')){//连接帧
						main_busy=1;
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
						 main_busy=0;
					}
 /*******************************************频谱扫描帧**********************************************************************************/					
					else if((USART_RX_BUF[1]=='f')&&(USART_RX_BUF[2]=='r')&&(USART_RX_BUF[3]=='e')&&(USART_RX_BUF[4]=='_')){//频谱扫描帧
						main_busy=1;
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
						index_frame_send+=4;

						for(j=0;j<FREQUENCY_POINT;j++){
							
							RDA5820_Freq_Set(freqset);//设置频率
							delay_ms(10);//等待10ms调频信号稳定
							if(RDA5820_RD_Reg(0X0B)&(1<<8)){//是一个有效电台. 
								frame_send_buf[9]=1;		 
							}else{
								frame_send_buf[9]=0;
							}
							delay_ms(10);
							frame_send_buf[8]=RDA5820_Rssi_Get();//得到信号强度
							frame_send_buf[10]=XOR(frame_send_buf,index_frame_send-1);
							for(t=0;t<index_frame_send;t++)
							{
								if(main_busy==0)break;
								USART_SendData(USART1, frame_send_buf[t]);//向串口发送数据
								while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
							}
							if(main_busy==0)break;
						//	delay_ms(10);
							frame_send_buf[7]++;

							if(freqset<FREQUENCY_MAX)freqset+=10;  //频率增加100Khz
							else freqset=FREQUENCY_MIN;		 	//越界处理 
						 }

						 RDA5820_TX_Mode();			//频谱扫描之后切换回发送模式
						 RDA5820_Freq_Set(send_frequency);	//设置频率，换为全局变量
						 delay_ms(20);					 
						 USART_RX_STA=0;//处理完毕，允许接收下一帧
						 main_busy=0;
					}
 /*******************************************控制帧**********************************************************************************/
					else if((USART_RX_BUF[1]=='c')&&(USART_RX_BUF[2]=='o')&&(USART_RX_BUF[3]=='n')&&(USART_RX_BUF[4]=='_')){//控制帧
						main_busy=1;
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
						 main_busy=0;
					}
 /*******************************************数据帧**********************************************************************************/					
					else if((USART_RX_BUF[1]=='d')&&(USART_RX_BUF[2]=='a')&&(USART_RX_BUF[3]=='t')&&(USART_RX_BUF[4]=='_')){//数据帧
						main_busy=1;
				//		frame_resent();测试请求重传
				//		delay_ms(100);
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
							USART_SendData(USART1, frame_send_buf[t]);//向串口发送数据
							while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
						}
					}else{
						flag_byte_ready=0;//数据帧字节流保存到本地，未成功
						fm_frame_index_byte=0;//字节流数组清空
						USART_RX_STA=0;
//						printf("\r\nData frame error.\r\n");
						}
					 main_busy=0;
					}else {USART_RX_STA=0;}//else{printf("Frame anomalous!");}
				}else {//数据帧的校验和出错时，请求重传
					USART_RX_STA=0;
					frame_resent();//求重传		
					}//else{printf("Verify bits wrong!");}
		    }else {USART_RX_STA=0;}//if(USART_RX_BUF[0]=='$')
						
		}//if(USART_RX_STA&0x8000)
			
	}//while(1)
}



void frame_resent(void){
	u16 t=0;

if((USART_RX_BUF[1]=='d')&&(USART_RX_BUF[2]=='a')&&(USART_RX_BUF[3]=='t')&&(USART_RX_BUF[4]=='_')){//数据帧
	index_frame_send=0;
	frame_send_buf[index_frame_send]='$';
	index_frame_send++;
	frame_send_buf[index_frame_send]='r';
	index_frame_send++;
	frame_send_buf[index_frame_send]='s';
	index_frame_send++;
	frame_send_buf[index_frame_send]='t';
	index_frame_send++;
	frame_send_buf[index_frame_send]='_';
	index_frame_send++;
	frame_send_buf[index_frame_send]='_';
	index_frame_send++;
	frame_send_buf[index_frame_send]=0;
	index_frame_send++;
	frame_send_buf[index_frame_send]=0;//重传数据帧类型。1：广播唤醒帧；2：单播唤醒帧；3：组播唤醒帧；4：控制帧；5：认证帧；
	index_frame_send++;
	frame_send_buf[index_frame_send]=XOR(frame_send_buf,index_frame_send);
	index_frame_send++;

//	fm_total_bytes=USART_RX_BUF[6]*256+USART_RX_BUF[7];
//	if((fm_total_bytes==FRAME_WAKEUP_BROADCAST )||(fm_total_bytes==FRAME_WAKEUP_UNICAST  )||(fm_total_bytes==FRAME_WAKEUP_MULTICAST  )||(fm_total_bytes==FRAME_CONTROL  )||(fm_total_bytes==FRAME_SECURTY  )){
//
//	}

	for(t=0;t<index_frame_send;t++)
	{
		USART_SendData(USART1, frame_send_buf[t]);//向串口发送数据
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
	}
	flag_byte_ready=0;//数据帧字节流保存到本地，未成功
	fm_frame_index_byte=0;//字节流数组清空


	}

}





