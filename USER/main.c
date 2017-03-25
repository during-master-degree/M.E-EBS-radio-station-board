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
#include "gray.h"
#include "stmflash.h"

#define FREQUENCY_MIN 7600
#define FREQUENCY_MAX 10100
#define FREQUENCY_POINT (FREQUENCY_MAX-FREQUENCY_MIN)/10+1
#define FRAME_WAKEUP_BROADCAST 120*2/4
#define FRAME_WAKEUP_UNICAST 144*2/4
#define FRAME_WAKEUP_MULTICAST 168*2/4
#define FRAME_CONTROL 84*2/4
#define FRAME_SECURTY 84/4

#define FLASH_SAVE_ADDR  0X08070000 //设置FLASH 保存地址(必须为偶数)
u8 TEXT_Buffer[4]={0};
#define SIZE sizeof(TEXT_Buffer)	 			  	//数组长度

u8 index_frame_send=0;//串口回复信息帧下标
u8 frame_send_buf[100]={0};//串口回传缓冲区

u16 fm_frame_index_bits=0;//FM广播01序列比特流下标
u8 fm_frame_bits[1100]={1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,1,1,0,0,0,1,0,0,1,0};//FM广播帧序列比特流缓冲区
u16 fm_frame_index_byte=0;//FM广播01序列转为字节流的下标
u8 fm_frame_byte[1100/2]={0};//FM广播帧序列字节流缓冲区

void frame_resent(void);//向上位机请求重传
void safe_soc(void);//向安全芯片重传或请求重传
u8 flag_byte_ready=0;//字节流已经全部保存到本地buffer标志位

//u8 temp_frame_byte[1100/2]={0};//测试数组
u16 temp_frame_byte_index=0;//测试数组

u8 usart2_works=0;//串口2工作状态指示。0：空闲；1：发送连接帧；2：接收连接反馈帧；3：发送数据帧；4：接收数据帧；
u8 usart1_works=0;//串口1工作状态指示。0：空闲；1：接收连接帧；2：发送连接反馈帧；3：接收频谱扫描帧；4：发送频谱扫描反馈帧；
				  //5:接收控制帧;6:发送控制反馈帧;7:接收数据帧;8:发送数据反馈帧(包括重传帧);9：数据帧无线传输中(不允许被打断)；
u8 frame_safe[40]={0};//认证帧重组，发给安全芯片
u8 index_frame_safe=0;//认证帧数组下表，发给安全芯片
u8 index_safe_times=0;//认证帧发送次数计数器
u8 flag_safe_frame=0;//本帧是认证帧
u8 flag_safe_soc_ok=0;//安全芯片超时与应答。1：未应答（不可用）；0：已应答（可用）；

u8 flag_is_wakeup_frame=0;//当前帧是唤醒帧的标志位
u8 wakeup_times=0;//唤醒帧发送的次数

u8 flag_voice_broad=0;//是否正在广播。0：没有广播；1：正在广播；
u16 send_frequency=FREQUENCY_MIN;//发射频点，只保存发射的频率 
extern u8 timer_67_stop;//定时器6,7被停止标志位。0：未被终止；1：被终止；

int main(void)
{	 
	u16 freqset=FREQUENCY_MIN;//接收、发射频点
	
	u16 fre_tmp=0;//计算频点的中间变量 
	u16 t=0,j=0;
	signed char i=0;//字节转比特流
	u16 len,len1;
	u16 fm_total_bytes=0;//数据帧总有效字节数量

	u16 safe_total_bytes=0;//安全帧总字节数
	u8 safe_mingwen[84]={0};//84bits明文消息的二进制码
	u8 safe_miwen[384]={0};//384bits安全芯片返回的密文消息
	u8 safe_mingwen_index=0;
	u16 safe_miwen_index=0;
	u8 before_gray[12]={0};//格雷编码前的12位的串
	u8 after_gray[24]={0};//格雷编码后的24位的串
	
	u8 xor_sum=0;
	u8 flash_temp[4]={0};
//	u8 power_send=0;//RDA发射功率

//	u8 bit_sync[14]={1,0,1,0,1,0,1,0,1,0,1,0,1,0};//位同步头
//	u8 frame_sync[11]={1,1,1,0,0,0,1,0,0,1,0};//帧同步头	
	tim3_pin_init();
	delay_init();	    	 //延时函数初始化	  
	NVIC_Configuration(); 	 //设置NVIC中断分组2:2位抢占优先级，2位响应优先级
	uart_init(115200);	 	//串口1初始化为9600
	uart2_init(9600);	 //串口2初始化为4800
//	KEY_Init();
 	LED_Init();			     //LED端口初始化
	TIM5_Int_Init(9999,7199);//安全芯片定期查询，1s中断一次，5秒查询一次
	TIM4_Int_Init(2999,7199);//安全芯片应答超时检测
	TIM3_Int_Init(9,7199);//1Khz的FSK方波
	TIM6_Int_Init(4,1199);//6K周期方波
	TIM7_Int_Init(4,719);//10k周期方波
	 
//	LCD_Init();			 	
// 	usmart_dev.init(72);	//初始化USMART	
// 	Audiosel_Init();
//	Audiosel_Set(AUDIO_RADIO);

	RDA5820_Init(); 	
	RDA5820_Band_Set(1);	//设置频段为0:87~108Mhz;1:76~91Mhz;
	RDA5820_Space_Set(0);	//设置步进为100Khz
	RDA5820_TxPGA_Set(1);	//信号增益设置为3
	RDA5820_TxPAG_Set(8);	//发射功率为最大.	
	RDA5820_RX_Mode();		//接收模式
	STMFLASH_Read(FLASH_SAVE_ADDR,(u16*)flash_temp,SIZE);
	fre_tmp=flash_temp[0]*10+FREQUENCY_MIN;
	
	if(flash_temp[2]>63){
		flash_temp[2]=8;//默认值这是为8，对应-13dbm
	}

	if((fre_tmp>FREQUENCY_MIN)&&(fre_tmp<FREQUENCY_MAX)){send_frequency=fre_tmp;}
	else{send_frequency=FREQUENCY_MIN;}
	RDA5820_Freq_Set(send_frequency);	//设置频率
 	while(1)
	{
//		if(USART2_RX_STA&0x8000)
//		{					   
//			len=USART2_RX_STA&0x3fff;//得到此次接收到的数据长度
//			for(t=0;t<len;t++)
//			{
//				USART_SendData(USART2, USART2_RX_BUF[t]);  //向串口2发送数据
//				while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);//等待发送结束
//			}
//			printf("\r\n\r\n");  //插入换行
//			USART2_RX_STA=0;
//		}
/******************************************************************串口2接收数据************************************************************************************************/
		if(USART2_RX_STA&0x8000)//加usart2_works==0
		{			   
			len1=USART2_RX_STA&0x3fff;//得到此次接收到的数据长度

			if((USART2_RX_BUF[0]=='$')&&(len1>0)){
				xor_sum=XOR(USART2_RX_BUF,len1-1);
				if((xor_sum=='$')||(xor_sum==0x0d)){xor_sum++;}
				if(USART2_RX_BUF[len1-1]==xor_sum){
 					/*******************************************安全芯片连接反馈帧**********************************************************************************/
					if((USART2_RX_BUF[1]=='s')&&(USART2_RX_BUF[2]=='a')&&(USART2_RX_BUF[3]=='f')&&(USART2_RX_BUF[4]=='_')&&(USART2_RX_BUF[5]=='_')){//连接帧
						usart2_works=2;//接收反馈的连接帧
						flag_safe_soc_ok=0;//标记安全芯片可用
						LED0=1;//关闭警示灯
						USART2_RX_STA=0;//处理完毕，允许接收下一帧
						usart2_works=0;//处理完，标记空闲
						USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//打开中断
					}
 					/*******************************************安全芯片认证数据回传帧**********************************************************************************/
				 	else if((USART2_RX_BUF[1]=='e')&&(USART2_RX_BUF[2]=='c')&&(USART2_RX_BUF[3]=='c')&&(USART2_RX_BUF[4]=='_')&&(USART2_RX_BUF[5]=='_')){//数据帧
						usart2_works=4;//接收反馈的数据帧
						flag_safe_soc_ok=0;//标记安全芯片可用					
						LED0=1;//关闭警示灯
						safe_miwen_index=0;
						safe_total_bytes=USART2_RX_BUF[7]*256+USART2_RX_BUF[8];//得到密文总的长度
						for(t=0;t<safe_total_bytes;t++){//把密文拆成比特流
							for (i=3;i>=0;i--)
							{
								safe_miwen[safe_miwen_index]=((USART2_RX_BUF[t+9]-0x30)&(0x01<<i))?1:0; 
								safe_miwen_index++;
							}
						}
	///////////////////////////////////////////1/////////////////////////////////////////////////////
						USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//打开中断

						for(t=0;t<84;t++)
						 {
				//			USART_SendData(USART1, safe_mingwen[t]);//向串口发送数据
				//			while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
							printf("%d",safe_mingwen[t]);
						 }
						 for(t=0;t<384;t++)
						 {
				//			USART_SendData(USART1, safe_miwen[t]);//向串口发送数据
				//			while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
							printf("%d",safe_miwen[t]);
						 }
						 for(t=0;t<len1;t++)
						 {
							USART_SendData(USART1, USART2_RX_BUF[t]);//向串口发送数据
							while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
						 }
   ///////////////////////////////////////////1 end//////////////////////////////////////////////////////
						//明文密文开始格雷编码，组帧，发送
						fm_frame_index_bits=25;//FM比特流数组重置
						for (t=0;t<(safe_mingwen_index/12);t++)//明文，格雷编码次数
						{
							for (i=0;i<12;i++)
							{
								before_gray[i]=safe_mingwen[t*12+i];
							}
							gray_encode(before_gray,after_gray);
							for (i=0;i<24;i++)
							{
								fm_frame_bits[fm_frame_index_bits]=after_gray[i];
								fm_frame_index_bits++;
							}							
						}
						for (t=0;t<(safe_miwen_index/12);t++)//密文，格雷编码次数
						{
							for (i=0;i<12;i++)
							{
								before_gray[i]=safe_miwen[t*12+i];
							}
							gray_encode(before_gray,after_gray);
							for (i=0;i<24;i++)
							{
								fm_frame_bits[fm_frame_index_bits]=after_gray[i];
								fm_frame_index_bits++;
							}							
						}

						
						RDA5820_TX_Mode();			//发送模式
						RDA5820_Freq_Set(send_frequency);	//设置频率，换为全局变量
						TIM_Cmd(TIM5, DISABLE); //失能TIM5
						timer_67_stop=0;//打开FSK震荡
						TIM_Cmd(TIM6, ENABLE); //使能TIM6
						TIM_Cmd(TIM7, ENABLE); //使能TIM7
						delay_ms(20);//让FSK信号先起振起来
						TIM_Cmd(TIM3, ENABLE); //使能TIM3
											

						USART2_RX_STA=0;//处理完毕，允许接收下一帧。防止循环进入
					}
 					/*******************************************安全芯片请求重传**********************************************************************************/
				 	else if((USART2_RX_BUF[1]=='r')&&(USART2_RX_BUF[2]=='t')&&(USART2_RX_BUF[3]=='s')&&(USART2_RX_BUF[4]=='_')&&(USART2_RX_BUF[5]=='_')){//重传帧
					   flag_safe_soc_ok=0;//标记安全芯片可用
						LED0=1;//关闭警示灯
						safe_soc();
						USART2_RX_STA=0;//处理完毕，允许接收下一帧
						usart2_works=0;//处理完，标记空闲
						USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//打开中断

					}else {safe_soc();USART2_RX_STA=0;USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);usart2_works=0;}//帧类型出错,请求重传
				 }else{safe_soc();USART2_RX_STA=0;USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);usart2_works=0;}//end of XOR，XOR出错请求重传
				 }else {USART2_RX_STA=0;USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);usart2_works=0;}//end of check '$'


		}else {USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);}//此处等待串口2回传数据，故usart2_works不能清零，但要添加对其为0的条件判断




/**************************************************串口1接收的数据帧，发送处理************************************************************************/
		if(flag_byte_ready==1){//数据保存本地成功
			usart1_works=9;//数据无线传输中
			if(flag_safe_frame==1){//认证帧				
				flag_safe_soc_ok=1;//标记为安全芯片未应答
				LED0=0;//打开警示灯
				usart2_works=3;//标记串口2开始发送完明文
				safe_soc();//把明文发送给安全芯片
				usart2_works=0;//标记串口2刚发送完明文	
				TIM4->ARR=2999;//重新状态定时器的值,300ms超时查询
				TIM_Cmd(TIM4,ENABLE);//打开超时判断定时器
				
				safe_mingwen_index=0;//将明文拆为比特流
				for(t=0;t<21;t++){
					for (i=3;i>=0;i--)
					{
						safe_mingwen[safe_mingwen_index]=(fm_frame_byte[t]&(0x01<<i))?1:0; 
						safe_mingwen_index++;
					}
				}
			}else{			
			
				fm_frame_index_bits=25;//FM比特流数组重置
				for(t=0;t<fm_frame_index_byte;t++){
					for (i=3;i>=0;i--)
					{
						fm_frame_bits[fm_frame_index_bits]=(fm_frame_byte[t]&(0x01<<i))?1:0; 
						fm_frame_index_bits++;
					}
				}
				RDA5820_TX_Mode();//发送模式
				RDA5820_Freq_Set(send_frequency);	//设置频率，换为全局变量
				TIM_Cmd(TIM5, DISABLE); //失能TIM5，避免产生16ms的中断干扰无线发送
				timer_67_stop=0;//打开FSK震荡
				TIM_Cmd(TIM6, ENABLE); //使能TIMx
				TIM_Cmd(TIM7, ENABLE); //使能TIMx
				delay_ms(20);//让FSK信号先起振起来
				TIM_Cmd(TIM3, ENABLE); //使能TIMx			
				
			}
//			for (temp_frame_byte_index=0;temp_frame_byte_index<fm_frame_index_bits/4;temp_frame_byte_index++)
//			{
//				temp_frame_byte[temp_frame_byte_index]=fm_frame_bits[temp_frame_byte_index*4]*8+fm_frame_bits[temp_frame_byte_index*4+1]*4+fm_frame_bits[temp_frame_byte_index*4+2]*2+fm_frame_bits[temp_frame_byte_index*4+3]*1+0x30;//ASCII 0码对应十进制是0x30
//			}

//			for(t=0;t<fm_frame_index_bits;t++)
//			{
//				USART_SendData(USART2, fm_frame_bits[t]);//向串口发送数据
//				while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);//等待发送结束
//			}
			
			flag_safe_frame=0;//清除认证帧标志位
			flag_byte_ready=0;//处理完毕，清零
		}
		
			
/**************************************串口1接收数据********************************************************************************************/
	   if((USART_RX_STA&0x8000)&&(flag_byte_ready==0)&&(usart1_works==0))
		{					   
			len=USART_RX_STA&0x3fff;//得到此次接收到的数据长度

			if((USART_RX_BUF[0]=='$')&&(len>0)){
				xor_sum=XOR(USART_RX_BUF,len-1);
				if((xor_sum=='$')||(xor_sum==0x0d)){xor_sum++;}
				if(USART_RX_BUF[len-1]==xor_sum){
					index_frame_send=0;
 					/*******************************************子板链接判断帧************************************************/
					if((USART_RX_BUF[1]=='r')&&(USART_RX_BUF[2]=='d')&&(USART_RX_BUF[3]=='y')&&(USART_RX_BUF[4]=='_')){//连接帧
						usart1_works=1;	//收到连接帧
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
						 STMFLASH_Read(FLASH_SAVE_ADDR,(u16*)flash_temp,SIZE);
						 frame_send_buf[index_frame_send]=flash_temp[0];//相对频点
						 index_frame_send++;

						 if(flash_temp[2]>64)flash_temp[2]=8;//值超过范围，设置为默认值8，对应-13dbm
						 RDA5820_TxPAG_Set(flash_temp[2]);

						 frame_send_buf[index_frame_send]=flash_temp[2];//发射功率
						 index_frame_send++;
						 frame_send_buf[index_frame_send]=XOR(frame_send_buf,index_frame_send);
						 index_frame_send++;
						 usart1_works=2;//发送连接反馈帧
						 for(t=0;t<index_frame_send;t++)
						 {
							USART_SendData(USART1, frame_send_buf[t]);//向串口发送数据
							while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
						 }
						 //printf("\r\n子板连接帧\r\n");
						 USART_RX_STA=0;//处理完毕，允许接收下一帧
						 USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//打开中断
						 usart1_works=0;//处理完，标志空闲
					}
 					/*******************************************频谱扫描帧*************************************************************/					
					else if((USART_RX_BUF[1]=='f')&&(USART_RX_BUF[2]=='r')&&(USART_RX_BUF[3]=='e')&&(USART_RX_BUF[4]=='_')){//频谱扫描帧
						USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//频谱扫描帧发送过程中允许被中断，打开中断
						usart1_works=3;//接收到频谱扫描帧
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

						usart1_works=4;//发送频谱扫描反馈帧
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
								if(usart1_works==0){
									break;
								}
								USART_SendData(USART1, frame_send_buf[t]);//向串口发送数据
								while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
							}
							if(usart1_works==0){

								break;
							}
						//	delay_ms(10);
							frame_send_buf[7]++;

							if(freqset<FREQUENCY_MAX)freqset+=10;  //频率增加100Khz
							else freqset=FREQUENCY_MIN;		 	//越界处理 
						 }

						 if(flag_voice_broad==1) RDA5820_TX_Mode();			//频谱扫描之后，如果之前是在广播语音，则切换回发送模式
						 RDA5820_Freq_Set(send_frequency);	//设置频率，换为全局变量
						 delay_ms(20);					 
						 USART_RX_STA=0;//处理完毕，允许接收下一帧
						 usart1_works=0;//处理完，标志清零
					}
 					/*******************************************控制帧*************************************************************/
					else if((USART_RX_BUF[1]=='c')&&(USART_RX_BUF[2]=='o')&&(USART_RX_BUF[3]=='n')&&(USART_RX_BUF[4]=='_')){//控制帧
						usart1_works=5;//接收到控制帧
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
						 frame_send_buf[index_frame_send]=USART_RX_BUF[6];
						 index_frame_send++;
						 frame_send_buf[index_frame_send]=XOR(frame_send_buf,index_frame_send);
						 index_frame_send++;
						 switch(USART_RX_BUF[6]){
						 	case 1:
								break;
							case 2:
								send_frequency=USART_RX_BUF[7]*10+FREQUENCY_MIN;
								TEXT_Buffer[0]=USART_RX_BUF[7];
								STMFLASH_Write(FLASH_SAVE_ADDR,(u16*)TEXT_Buffer,SIZE);
								RDA5820_Freq_Set(send_frequency);	//设置频率，换为全局变量
								break;
							case 3:
								flag_voice_broad=1;//开始广播语音
								RDA5820_TX_Mode();
								RDA5820_Freq_Set(send_frequency);	//设置频率，换为全局变量
						 		delay_ms(20);
								break;
							case 4:
								flag_voice_broad=0;//停止广播语音
								RDA5820_RX_Mode();
								RDA5820_Freq_Set(send_frequency);	//设置频率，换为全局变量
								delay_ms(20);
								break;
							case 5://调整发射功率
						//		RDA5820_TxPGA_Set(4);	//信号增益设置为3
								TEXT_Buffer[2]=USART_RX_BUF[7]-13;
								STMFLASH_Write(FLASH_SAVE_ADDR,(u16*)TEXT_Buffer,SIZE);
								RDA5820_TxPAG_Set((USART_RX_BUF[7]-13));	//发射功率为最大.//将USART_RX_BUF[7]抬高到13以上，避免出现帧尾0x0d
								break;
							default:
							break;
						 }
						 usart1_works=6;//发送控制反馈帧
						 for(t=0;t<index_frame_send;t++)
						 {
							USART_SendData(USART1, frame_send_buf[t]);//向串口发送数据
							while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
						 }
						 //printf("\r\n控制帧\r\n");
						 USART_RX_STA=0;//处理完毕，允许接收下一帧
						 USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//打开中断
						 usart1_works=0;//处理完毕，标志清零
					}
 					/*******************************************数据帧*************************************************************/					
					else if((USART_RX_BUF[1]=='d')&&(USART_RX_BUF[2]=='a')&&(USART_RX_BUF[3]=='t')&&(USART_RX_BUF[4]=='_')){//数据帧
						usart1_works=7;//接收到数据帧
				//		frame_resent();测试请求重传

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
							if((fm_total_bytes==FRAME_WAKEUP_BROADCAST)||(fm_total_bytes==FRAME_WAKEUP_UNICAST)||(fm_total_bytes==FRAME_WAKEUP_MULTICAST)||(fm_total_bytes==FRAME_CONTROL)){
								fm_frame_byte[fm_frame_index_byte]=USART_RX_BUF[t+9]-0x30;//唤醒帧中加入了一个字节，所以唤醒帧与认证帧的数据起始位没有对齐
							}else{fm_frame_byte[fm_frame_index_byte]=USART_RX_BUF[t+8]-0x30;}//认证帧
							fm_frame_index_byte++;
						}
						if(fm_total_bytes==FRAME_SECURTY){flag_safe_frame=1;}//收到了认证帧
						if((fm_total_bytes==FRAME_WAKEUP_BROADCAST)||(fm_total_bytes==FRAME_WAKEUP_UNICAST)||(fm_total_bytes==FRAME_WAKEUP_MULTICAST)){
							flag_is_wakeup_frame=1;//收到了唤醒帧
							wakeup_times=(USART_RX_BUF[8]-0x30)*3;//每秒钟发送三次唤醒帧
						}else if(fm_total_bytes==FRAME_CONTROL){
							flag_is_wakeup_frame=2;//收到了控制帧
							wakeup_times=(USART_RX_BUF[8]-0x30)*3;//每秒钟发送三次唤醒帧
						}
						flag_byte_ready=1;//数据帧字节流保存到本地，成功
				   /*******************数据帧处理，1字节转为4bits，结束*********************************/
				   		usart1_works=8;//发送数据反馈帧		
						for(t=0;t<index_frame_send;t++)
						{
							USART_SendData(USART1, frame_send_buf[t]);//向串口发送数据
							while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
						}
					}else{//这里应该不会执行，保险起见，对之处理
						flag_byte_ready=0;//数据帧字节流保存到本地，未成功
						flag_safe_frame=0;//取消安全帧标志位
						fm_frame_index_byte=0;//字节流数组清空
						USART_RX_STA=0;
						usart1_works=0;//异常，标志清零
						USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);	
//						printf("\r\nData frame error.\r\n");
						}
					}else {USART_RX_STA=0;USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);usart1_works=0;}//else{printf("Frame anomalous!");}
				}else {//帧的校验和出错时，是数据帧的校验和出错时，请求重传
					USART_RX_STA=0;
					USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//打开中断
					frame_resent();//求重传
					usart1_works=0;		
				}//else{printf("Verify bits wrong!");}
		    }else {USART_RX_STA=0;USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);usart1_works=0;}//if(USART_RX_BUF[0]=='$')
						
		}else if((flag_byte_ready==0)&&(usart1_works==0)){USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);}//if(USART_RX_STA&0x8000)，本判断只有当数据帧字节流未收满时使用；除数据帧外的其他帧在本判断中已处理完毕
			
	}//while(1)
}



void frame_resent(void){
	u8 t=0;
	
if((USART_RX_BUF[1]=='d')&&(USART_RX_BUF[2]=='a')&&(USART_RX_BUF[3]=='t')&&(USART_RX_BUF[4]=='_')||
	(USART_RX_BUF[1]=='f')&&(USART_RX_BUF[2]=='r')&&(USART_RX_BUF[3]=='e')&&(USART_RX_BUF[4]=='_')||
	(USART_RX_BUF[1]=='c')&&(USART_RX_BUF[2]=='o')&&(USART_RX_BUF[3]=='n')&&(USART_RX_BUF[4]=='_')
	){//连接帧除外,帧校验和出错就重传
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
	frame_send_buf[index_frame_send]=0;//一问一答方式，上位机自动重传刚才发的数据帧。重传数据帧类型。1：广播唤醒帧；2：单播唤醒帧；3：组播唤醒帧；4：控制帧；5：认证帧；
	index_frame_send++;
	frame_send_buf[index_frame_send]=XOR(frame_send_buf,index_frame_send);
	index_frame_send++;

	for(t=0;t<index_frame_send;t++)
	{
		USART_SendData(USART1, frame_send_buf[t]);//向串口发送数据
		while(USART_GetFlagStatus(USART1,USART_FLAG_TC)!=SET);//等待发送结束
	}
	flag_byte_ready=0;//数据帧字节流保存到本地，未成功
	fm_frame_index_byte=0;//字节流数组清空


	}

}

void safe_soc(void){
	u16 t=0;
	index_frame_safe=0;
	frame_safe[index_frame_safe]='$';
	index_frame_safe++;
	frame_safe[index_frame_safe]='d';
	index_frame_safe++;
	frame_safe[index_frame_safe]='a';
	index_frame_safe++;
	frame_safe[index_frame_safe]='t';
	index_frame_safe++;
	frame_safe[index_frame_safe]='_';
	index_frame_safe++;
	if (index_safe_times<200)
	{
		index_safe_times++;
		if(((index_safe_times==0x0d)||(index_safe_times==0x24))){
			index_safe_times++;
		}
	} 
	else
	{
		index_safe_times=0;
	}
	frame_safe[index_frame_safe]=index_safe_times;
	index_frame_safe++;
	frame_safe[index_frame_safe]=(84/4)/256;
	index_frame_safe++;
	frame_safe[index_frame_safe]=(84/4)%256;
	index_frame_safe++;
	for(t=0;t<fm_frame_index_byte;t++){
		frame_safe[index_frame_safe]=fm_frame_byte[t]+0x30;
		index_frame_safe++;
	}
	frame_safe[index_frame_safe]=XOR(frame_safe,index_frame_safe);
	if((frame_safe[index_frame_safe]=='$')||(frame_safe[index_frame_safe]==0x0d)){frame_safe[index_frame_safe]++;}
	index_frame_safe++;
	frame_safe[index_frame_safe]='\r';
	index_frame_safe++;
	frame_safe[index_frame_safe]='\n';
	index_frame_safe++;
	USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//暂时打开，只为调试
	for(t=0;t<index_frame_safe;t++)//认证帧通过串口2发送给安全芯片
	{
		USART_SendData(USART2, frame_safe[t]);//向串口发送数据
		while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);//等待发送结束
	}
	
	

}



