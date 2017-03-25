#include "timer.h"
#include "led.h"
#include "usart.h"

//////////////////////////////////////////////////////////////////////////////////	 
//本程序只供学习使用，未经作者许可，不得用于其它任何用途
//ALIENTEK战舰STM32开发板
//定时器 驱动代码	   
//正点原子@ALIENTEK
//技术论坛:www.openedv.com
//修改日期:2012/9/3
//版本：V1.0
//版权所有，盗版必究。
//Copyright(C) 广州市星翼电子科技有限公司 2009-2019
//All rights reserved									  
//////////////////////////////////////////////////////////////////////////////////   	 

//通用定时器3中断初始化
//这里时钟选择为APB1的2倍，而APB1为36M
//arr：自动重装值。
//psc：时钟预分频数
//这里使用的是定时器3!
void TIM3_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE); //时钟使能
	
	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM3,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM3_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器


	TIM_Cmd(TIM3, DISABLE);  //关闭TIMx					 
}



u8 frame_wakeup_broadcast[]={1,0,1,0,1,0,1,0,1,0,1,0,1,0,
1,1,1,0,0,0,1,0,0,1,0,
0,1,0,0,
0,0,1,0,
0,1,1,0,0,0,1,1,0,1,0,1,1,1,1,1,
1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,
1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,
0,1,0,0,1,1,1,1,0,1,1,0,1,1,1,0,0,1,1,1,1,0,1,1,0,0,1,0,1,1,0,1,1,0,1,1,1,1,1,0,0,1,0,1,1,1,1,1,0,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,0,1,0,0,1,1,0};

u8 frame_wakeup_unicast[]={1,0,1,0,1,0,1,0,1,0,1,0,1,0,
1,1,1,0,0,0,1,0,0,1,0, 
0,1,0,0,
0,0,1,0,
1,0,1,0,0,1,0,0,0,0,0,0,1,1,1,0,
1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0, 
0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,
1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,
0,0,0,0,1,0,1,0,0,1,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,1,0,1,1,0,0,1,1,1,0,0,1,0,1,0,1,1,1,0,0,1,1,0,1,0,0,1,1,0,1,1,0,1,0,1,1,1,1,1};

u8 frame_wakeup_multicast[]={1,0,1,0,1,0,1,0,1,0,1,0,1,0,
1,1,1,0,0,0,1,0,0,1,0,
0,1,0,0,
0,0,1,0,
1,1,1,0,1,1,1,0,1,0,0,1,1,0,0,1,
1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0, 
0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0, 
1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0, 
0,1,1,0,1,0,1,0,0,0,0,0,1,1,0,0,0,1,0,0,0,1,0,0,1,0,0,1,0,0,1,1,0,1,0,1,1,1,0,0,0,0,0,1,0,1,1,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,0,0,0,1,1,1,1,1,1}; 

u8 frame_control[]={1,0,1,0,1,0,1,0,1,0,1,0,1,0,
1,1,1,0,0,0,1,0,0,1,0,
1,0,0,0,
0,0,0,0,0,0,0,1,0,1,0,0,1,0,0,1,1,1,1,0,
1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,
0,1,0,0,0,1,0,1,0,1,0,0,0,0,1,0,0,1,0,1,0,0,1,0,0,1,0,0,0,0,0,1,1,1,0,1,1,0,1,0,1,1,1,0,0,1,1,0,1,1,1,1,0,0,1,0,0,0,1,1,0,1,0,1,1,0,0,0,1,0,1,0};


u16 t=0;//发送数组下标计数器
extern u16 fm_frame_index_bits;//FM广播01序列比特流下标
extern u8 fm_frame_bits[1100];//FM广播帧序列比特流缓冲区
//extern u8 flag_frame_processing;//收到的数据帧正在处理标志位。1:处理中;0:空闲;
extern u8 usart2_works;//串口2工作状态指示。0：空闲；1：发送连接帧；2：接收连接反馈帧；3：发送数据帧；4：接收数据帧(包括重传帧)；
extern u8 usart1_works;//串口1工作状态指示。0：空闲；1：接收连接帧；2：发送连接反馈帧；3：接收频谱扫描帧；4：发送频谱扫描反馈帧；
				  //5:接收控制帧;6:发送控制反馈帧;7:接收数据帧;8:发送数据反馈帧(包括重传帧);9：数据帧无线传输中；
//定时器3中断服务程序
void TIM3_IRQHandler(void)   //TIM3中断
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  //检查TIM3更新中断发生与否
	{	
//		if(t<fm_frame_index_bits){
//			PBout(6)=fm_frame_bits[t];//还需要加位同步，帧同步头
//			t++;
//		}
		if(t<fm_frame_index_bits)//广播25+120*2；单播25+144*2；组播25+168*2；控制帧：25+84*2；
		{
			PBout(6)=fm_frame_bits[t];//frame_control[t];
			t++;
		}else//帧发送完毕后的清理
		{
			PBout(6)=0;
			t=0;
			TIM_Cmd(TIM3,DISABLE);
			TIM_Cmd(TIM6,DISABLE);
			TIM_Cmd(TIM7,DISABLE);
//			flag_frame_processing=0;
			usart1_works=0;//处理完毕，标志串口1允许使用
			USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//打开中断
		}//数据帧循环发送
		

		TIM_ClearITPendingBit(TIM3, TIM_IT_Update  );  //清除TIMx更新中断标志
	}
}


void TIM5_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE); //时钟使能
	
	//定时器TIM5初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM5,TIM_IT_Update,ENABLE ); //使能指定的TIM5中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 2;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器


	TIM_Cmd(TIM5, ENABLE);  //打开TIMx					 
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



u8 secury_chip_ckeck=0;//安全芯片查询位，计数到5即5秒，查询一次
u8 frame_send_buf_chip[100]={0};//串口回传缓冲区
u8 index_chipcheck_times=0;//芯片查询次数
extern u8 flag_safe_soc;//安全芯片是否可用标志位。0：可用；1：不可用；

void TIM5_IRQHandler(void)   //TIM5中断
{
	u16 t=0;
	u8 index_frame_send_chip=0;//串口回复信息帧下标

	if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET)  //检查TIM5更新中断发生与否
	{
		TIM_ClearITPendingBit(TIM5, TIM_IT_Update  );  //清除TIMx更新中断标志
		LED1=!LED1;
		if(flag_safe_soc==0){//安全芯片可用时 
		
		if((secury_chip_ckeck>=CHIP_CHECK_FREQUENCY)&&(usart1_works==0)&&(usart2_works==0)){//计数到5秒钟，当串口1、2都空闲时，开始查询，否则等待下一次中断时重新判断
			frame_send_buf_chip[index_frame_send_chip]='$';
			index_frame_send_chip++;
			frame_send_buf_chip[index_frame_send_chip]='s';
			index_frame_send_chip++;
			frame_send_buf_chip[index_frame_send_chip]='a';
			index_frame_send_chip++;
			frame_send_buf_chip[index_frame_send_chip]='f';
			index_frame_send_chip++;
			frame_send_buf_chip[index_frame_send_chip]='_';
			index_frame_send_chip++;
			if(index_chipcheck_times>200){index_chipcheck_times=0;}
			else index_chipcheck_times++;
			frame_send_buf_chip[index_frame_send_chip]=index_chipcheck_times;
			index_frame_send_chip++;
			frame_send_buf_chip[index_frame_send_chip]=XOR(frame_send_buf_chip,index_frame_send_chip);
			index_frame_send_chip++;
			frame_send_buf_chip[index_frame_send_chip]='\r';
			index_frame_send_chip++;
			frame_send_buf_chip[index_frame_send_chip]='\n';
			index_frame_send_chip++;
			for(t=0;t<index_frame_send_chip;t++)
			{
				USART_SendData(USART2, frame_send_buf_chip[t]);  //向串口2发送数据
				while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);//等待发送结束
			}
			flag_safe_soc=1;//查询前先标记不可用，串口2收到应答后再标记为可用
			secury_chip_ckeck=0;		
		}else{
			secury_chip_ckeck++;
		}
	   }else{//检测到上次置的不可用状态维持到了第二次查询，认为安全芯片故障
	   	    LED0=0;
	   }
	}
}


/***************************FSK*****************************/
void TIM6_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM6, ENABLE); //时钟使能
	
	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM6, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM6,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM6_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器


	TIM_Cmd(TIM6, DISABLE);  //关闭TIMx							 
}

void TIM7_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM7, ENABLE); //时钟使能
	
	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM7, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM7,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM7_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器


	TIM_Cmd(TIM7, DISABLE);  //关闭TIMx							 
}

void tim3_pin_init(void){
     GPIO_InitTypeDef  GPIO_InitStructure;
	 RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB|RCC_APB2Periph_GPIOA, ENABLE);//使能PB,端口时钟
	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_8|GPIO_Pin_9;				 //LED0-->PB.5 端口配置
	 GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP; 		 //推挽输出
	 GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;		 //IO口速度为50MHz
	 GPIO_Init(GPIOB, &GPIO_InitStructure);

	 GPIO_InitStructure.GPIO_Pin = GPIO_Pin_7;				 //LED0-->PB.5 端口配置
	 GPIO_Init(GPIOA, &GPIO_InitStructure);
	 GPIO_ResetBits(GPIOB,GPIO_Pin_8); //讨厌的蜂鸣器
}

//定时器6中断服务程序
unsigned int index_tim6=0;
void TIM6_IRQHandler(void)   //TIM3中断
{
		if (TIM_GetITStatus(TIM6, TIM_IT_Update) != RESET)  //检查TIM3更新中断发生与否
		{
		TIM_ClearITPendingBit(TIM6, TIM_IT_Update);  //清除TIMx更新中断标志 
//			if(index_tim6<(19+511-10))
//			{
				PBout(8)=~PBout(8);
//				index_tim6++;
//				
//			}else//帧发送完毕后的清理
//			{
//				PBout(8)=0;
//				index_tim6=0;			
//			}//数据帧循环发送
				
		}
}

//定时器7中断服务程序
unsigned int index_tim7=0;
void TIM7_IRQHandler(void)   //TIM3中断
{
   	if (TIM_GetITStatus(TIM7, TIM_IT_Update) != RESET)  //检查TIM3更新中断发生与否
		{
		TIM_ClearITPendingBit(TIM7, TIM_IT_Update);  //清除TIMx更新中断标志 
//			if(index_tim7<(19+511-10))
//			{
				PBout(9)=~PBout(9);
//				index_tim7++;
//				
//			}else//帧发送完毕后的清理
//			{
//				PBout(9)=0;
//				index_tim7=0;			
//			}//数据帧循环发送
				
		}
}





