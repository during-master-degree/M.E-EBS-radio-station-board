#include "sys.h"
#include "usart.h"
#include "led.h"	  
////////////////////////////////////////////////////////////////////////////////// 	 
//如果使用ucos,则包括下面的头文件即可.
#if SYSTEM_SUPPORT_UCOS
#include "includes.h"					//ucos 使用	  
#endif
//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK STM32开发板
//串口1初始化		   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/8/18
//版本：V1.5
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved
//********************************************************************************
//V1.3修改说明 
//支持适应不同频率下的串口波特率设置.
//加入了对printf的支持
//增加了串口接收命令功能.
//修正了printf第一个字符丢失的bug
//V1.4修改说明
//1,修改串口初始化IO的bug
//2,修改了USART_RX_STA,使得串口最大接收字节数为2的14次方
//3,增加了USART_REC_LEN,用于定义串口最大允许接收的字节数(不大于2的14次方)
//4,修改了EN_USART1_RX的使能方式
//V1.5修改说明
//1,增加了对UCOSII的支持
////////////////////////////////////////////////////////////////////////////////// 	  
 

//////////////////////////////////////////////////////////////////
//加入以下代码,支持printf函数,而不需要选择use MicroLIB	  
#if 1
#pragma import(__use_no_semihosting)             
//标准库需要的支持函数                 
struct __FILE 
{ 
	int handle; 

}; 

FILE __stdout;       
//定义_sys_exit()以避免使用半主机模式    
_sys_exit(int x) 
{ 
	x = x; 
} 
//重定义fputc函数 
int fputc(int ch, FILE *f)
{      
	while((USART1->SR&0X40)==0);//循环发送,直到发送完毕   
    USART1->DR = (u8) ch;      
	return ch;
}
#endif 

/*使用microLib的方法*/
 /* 
int fputc(int ch, FILE *f)
{
	USART_SendData(USART1, (uint8_t) ch);

	while (USART_GetFlagStatus(USART1, USART_FLAG_TC) == RESET) {}	
   
    return ch;
}
int GetKey (void)  { 

    while (!(USART1->SR & USART_FLAG_RXNE));

    return ((int)(USART1->DR & 0x1FF));
}
*/
 
#if EN_USART1_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   	
u8 USART_RX_BUF[USART_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART_RX_STA=0;       //接收状态标记	  

//初始化IO 串口1 
//bound:波特率
void uart_init(u32 bound){
    //GPIO端口设置
    GPIO_InitTypeDef GPIO_InitStructure;
	USART_InitTypeDef USART_InitStructure;
	NVIC_InitTypeDef NVIC_InitStructure;
	 
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_USART1|RCC_APB2Periph_GPIOA, ENABLE);	//使能USART1，GPIOA时钟
 	USART_DeInit(USART1);  //复位串口1
	 //USART1_TX   PA.9
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9; //PA.9
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
    GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA9
   
    //USART1_RX	  PA.10
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_10;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
    GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA10

   //Usart1 NVIC 配置

    NVIC_InitStructure.NVIC_IRQChannel = USART1_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
	NVIC_Init(&NVIC_InitStructure);	//根据指定的参数初始化VIC寄存器
  
   //USART 初始化设置

	USART_InitStructure.USART_BaudRate = bound;//一般设置为9600;
	USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
	USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
	USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
	USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
	USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式

    USART_Init(USART1, &USART_InitStructure); //初始化串口
    USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//开启中断
    USART_Cmd(USART1, ENABLE);//使能串口 

}


u8 flag_frame_sync=0;//串口1数据帧同步标志位
extern u8 usart2_works;//串口2工作状态指示。0：空闲；1：发送连接帧；2：接收连接反馈帧；3：发送数据帧；4：接收数据帧(包括重传帧)；
extern u8 usart1_works;//串口1工作状态指示。0：空闲；1：接收连接帧；2：发送连接反馈帧；3：接收频谱扫描帧；4：发送频谱扫描反馈帧；
				  //5:接收控制帧;6:发送控制反馈帧;7:接收数据帧;8:发送数据反馈帧(包括重传帧);9：数据帧无线传输中；

void USART1_IRQHandler(void)                	//串口1中断服务程序
	{
	u8 Res;
//#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
//	OSIntEnter();    
//#endif


	if(USART_GetITStatus(USART1, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
		{
		Res =USART_ReceiveData(USART1);//(USART1->DR);	//读取接收到的数据
		if(Res=='$'){flag_frame_sync=1;USART_RX_STA=0;}//一帧数据中多次出现$，则从最后的一个$开始真正存储；可以防止结尾未出现0x0d,0x0a而空等待的问题
		if(flag_frame_sync==1){//开始本次接收
		if(usart1_works==4){//频谱扫描中，允许被中断
			usart1_works=0;
			USART_RX_STA=0;
		}

		if(((USART_RX_STA&0x8000)==0)&&(usart1_works==0))//接收未完成
		{
			 if(USART_RX_STA&0x4000)//接收到了0x0d
				{
				if(Res!=0x0a)USART_RX_STA=0;//接收错误,重新开始
				else {
					USART_RX_STA|=0x8000;	//接收完成了
					flag_frame_sync=0;//准备下次接收
					USART_ITConfig(USART1, USART_IT_RXNE, DISABLE);//关闭中断
					} 
				}
			 else //还没收到0X0D
				{	
				if(Res==0x0d)USART_RX_STA|=0x4000;
				else
					{
					USART_RX_BUF[USART_RX_STA&0X3FFF]=Res ;
					USART_RX_STA++;
					if(USART_RX_STA>(USART_REC_LEN-1))USART_RX_STA=0;//接收数据错误,重新开始接收	  
					}		 
				}
		}
		
		}//end of $   		 
     } 
//#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
//	OSIntExit();  											 
//#endif
} 
#endif	






/*****************************串口2********************************/
#if EN_USART2_RX   //如果使能了接收
//串口1中断服务程序
//注意,读取USARTx->SR能避免莫名其妙的错误   							    
u8 USART2_RX_BUF[USART2_REC_LEN];     //接收缓冲,最大USART_REC_LEN个字节.	上位机给STM32发送东西
//接收状态
//bit15，	接收完成标志
//bit14，	接收到0x0d
//bit13~0，	接收到的有效字节数目
u16 USART2_RX_STA=0;       //接收状态标记	  

//初始化IO 串口2 
//bound:波特率
void uart2_init(u32 bound)
	{
	    //GPIO端口设置
	    GPIO_InitTypeDef GPIO_InitStructure;
		USART_InitTypeDef USART_InitStructure;
		NVIC_InitTypeDef NVIC_InitStructure;
		 
		RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);	//使能GPIOA时钟
		RCC_APB1PeriphClockCmd(RCC_APB1Periph_USART2, ENABLE);	//使能USART2
	 	USART_DeInit(USART2);  //复位串口1
		 //USART2_TX   PA.2
	    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_2; //PA.2
	    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;	//复用推挽输出
	    GPIO_Init(GPIOA, &GPIO_InitStructure); //初始化PA2
	   
	    //USART2_RX	  PA.3
	    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_3;
	    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;//浮空输入
	    GPIO_Init(GPIOA, &GPIO_InitStructure);  //初始化PA3
	
	
	   //USART 初始化设置
	
		USART_InitStructure.USART_BaudRate = bound;//一般设置为4800;
		USART_InitStructure.USART_WordLength = USART_WordLength_8b;//字长为8位数据格式
		USART_InitStructure.USART_StopBits = USART_StopBits_1;//一个停止位
		USART_InitStructure.USART_Parity = USART_Parity_No;//无奇偶校验位
		USART_InitStructure.USART_HardwareFlowControl = USART_HardwareFlowControl_None;//无硬件数据流控制
		USART_InitStructure.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;	//收发模式
	
	    USART_Init(USART2, &USART_InitStructure); //初始化串口
	    #if EN_USART2_RX		//如果使能了接收  
	    //Usart2 NVIC 配置
	    NVIC_InitStructure.NVIC_IRQChannel = USART2_IRQn;
		NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority=3 ;//抢占优先级3
		NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;		//子优先级3
		NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;			//IRQ通道使能
		NVIC_Init(&NVIC_InitStructure);	 //根据指定的参数初始化VIC寄存器
	   
	    USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//开启中断    等待RXNE置1
	    #endif
	    USART_Cmd(USART2, ENABLE);                    //使能串口 
	
	}

u8 flag_frame_sync_usart2=0;//串口2数据帧同步标志位
extern u8 flag_safe_soc_ok;//安全芯片是否可用标志位。0：可用；1：不可用；
void USART2_IRQHandler(void)                	//串口2中断服务程序
	{
	u8 Res;		//只可以存放一个字节的数据
//	#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
//		OSIntEnter();    
//	#endif
	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
		{
		Res =USART_ReceiveData(USART2);//(USART1->DR);	//读取接收到的数据
		if(Res=='$'){flag_frame_sync_usart2=1;USART2_RX_STA=0;}//一帧数据中多次出现$，则从最后的一个$开始真正存储；可以防止结尾未出现0x0d,0x0a而空等待的问题
		if(flag_frame_sync_usart2==1){//开始本次接收
			
//		if(main_busy==1){//认证帧组帧时，不允许被新帧打断
//			main_busy=0;
//			USART_RX_STA=0;
//		}

		if(((USART2_RX_STA&0x8000)==0)&&((usart2_works==0)||(usart2_works==1)))//接收未完成
		{
			 if(USART2_RX_STA&0x4000)//接收到了0x0d
				{
				if(Res!=0x0a)USART2_RX_STA=0;//接收错误,重新开始
				else {
					USART2_RX_STA|=0x8000;	//接收完成了
					flag_frame_sync_usart2=0;//准备下次接收
					USART_ITConfig(USART2, USART_IT_RXNE, DISABLE);//关闭中断
					} 
				}
			 else //还没收到0X0D
				{	
				if(Res==0x0d)USART2_RX_STA|=0x4000;
				else
					{
					USART2_RX_BUF[USART2_RX_STA&0X3FFF]=Res ;
					USART2_RX_STA++;
					if(USART2_RX_STA>(USART2_REC_LEN-1))USART2_RX_STA=0;//接收数据错误,重新开始接收	  
					}		 
				}
		}
		}//end of $   		 
     }

//#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
//	OSIntExit();  											 
//#endif
}


//void USART2_IRQHandler(void)                	//串口2中断服务程序
//	{
//	u8 Res;		//只可以存放一个字节的数据
//	#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
//		OSIntEnter();    
//	#endif
//	if(USART_GetITStatus(USART2, USART_IT_RXNE) != RESET)  //接收中断(接收到的数据必须是0x0d 0x0a结尾)
//		{
//			Res =USART_ReceiveData(USART2);//(USART2->DR);	//读取接收到的数据
//			
//			if((USART2_RX_STA&0x8000)==0)//接收未完成
//					{
//						if(USART2_RX_STA&0x40)//接收到了0x0d
//							{
//							if(Res!=0x0a) 
//							  USART2_RX_STA=0; //接收错误,重新开始
//							  else USART2_RX_STA|=0x80;	 //接收完成了 
//							}
//						  else //还没收到0X0D
//							{	
//							if(Res==0x0d)  USART2_RX_STA|=0x40;
//							else
//								{
//								USART2_RX_BUF[USART2_RX_STA&0X3F]=Res ;
//								USART2_RX_STA++;
//								if(USART2_RX_STA>(USART2_REC_LEN-1))
//								   USART2_RX_STA=0;//接收数据错误,重新开始接收	  
//								}		 
//							}
//					}   		 
//       } 
//#ifdef OS_TICKS_PER_SEC	 	//如果时钟节拍数定义了,说明要使用ucosII了.
//	OSIntExit();  											 
//#endif
//} 

 
#endif




