#include "timer.h"
#include "led.h"
#include "usart.h"
#include "delay.h"
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



//u8 frame_wakeup_broadcast[]={1,0,1,0,1,0,1,0,1,0,1,0,1,0,
//1,1,1,0,0,0,1,0,0,1,0,
//0,1,0,0,
//0,0,1,0,
//0,1,1,0,0,0,1,1,0,1,0,1,1,1,1,1,
//1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,
//1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,
//0,1,0,0,1,1,1,1,0,1,1,0,1,1,1,0,0,1,1,1,1,0,1,1,0,0,1,0,1,1,0,1,1,0,1,1,1,1,1,0,0,1,0,1,1,1,1,1,0,0,1,1,1,1,1,1,1,0,1,1,1,1,1,1,0,0,1,0,0,1,1,0};
//
//u8 frame_wakeup_unicast[]={1,0,1,0,1,0,1,0,1,0,1,0,1,0,
//1,1,1,0,0,0,1,0,0,1,0, 
//0,1,0,0,
//0,0,1,0,
//1,0,1,0,0,1,0,0,0,0,0,0,1,1,1,0,
//1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0, 
//0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,
//1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,
//0,0,0,0,1,0,1,0,0,1,0,0,1,1,1,1,1,0,0,0,0,0,0,0,0,0,0,0,1,0,1,1,1,1,0,1,1,0,0,1,1,1,0,0,1,0,1,0,1,1,1,0,0,1,1,0,1,0,0,1,1,0,1,1,0,1,0,1,1,1,1,1};
//
//u8 frame_wakeup_multicast[]={1,0,1,0,1,0,1,0,1,0,1,0,1,0,
//1,1,1,0,0,0,1,0,0,1,0,
//0,1,0,0,
//0,0,1,0,
//1,1,1,0,1,1,1,0,1,0,0,1,1,0,0,1,
//1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0, 
//0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0, 
//1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0, 
//0,1,1,0,1,0,1,0,0,0,0,0,1,1,0,0,0,1,0,0,0,1,0,0,1,0,0,1,0,0,1,1,0,1,0,1,1,1,0,0,0,0,0,1,0,1,1,1,0,0,0,0,1,1,0,0,0,1,0,1,1,1,0,0,0,0,1,1,1,1,1,1}; 
//
//u8 frame_control[]={1,0,1,0,1,0,1,0,1,0,1,0,1,0,
//1,1,1,0,0,0,1,0,0,1,0,
//1,0,0,0,
//0,0,0,0,0,0,0,1,0,1,0,0,1,0,0,1,1,1,1,0,
//1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,0,0,0,0,1,1,1,1,0,0,0,0,1,0,1,1,1,1,1,0,1,0,0,1,1,1,1,1,0,0,0,0,1,1,1,1,0,1,0,0,0,0,0,1,0,1,1,0,
//0,1,0,0,0,1,0,1,0,1,0,0,0,0,1,0,0,1,0,1,0,0,1,0,0,1,0,0,0,0,0,1,1,1,0,1,1,0,1,0,1,1,1,0,0,1,1,0,1,1,1,1,0,0,1,0,0,0,1,1,0,1,0,1,1,0,0,0,1,0,1,0};


u16 t=0;//发送数组下标计数器
extern u16 fm_frame_index_bits;//FM广播01序列比特流下标
extern u8 fm_frame_bits[1100];//FM广播帧序列比特流缓冲区
extern u8 usart2_works;//串口2工作状态指示。0：空闲；1：发送连接帧；2：接收连接反馈帧；3：发送数据帧；4：接收数据帧(包括重传帧)；
extern u8 usart1_works;//串口1工作状态指示。0：空闲；1：接收连接帧；2：发送连接反馈帧；3：接收频谱扫描帧；4：发送频谱扫描反馈帧；
				  //5:接收控制帧;6:发送控制反馈帧;7:接收数据帧;8:发送数据反馈帧(包括重传帧);9：数据帧无线传输中；
u8 timer_67_stop=0;//定时器6,7被停止标志位。0：未被终止；1：被终止；
extern u16 USART_RX_STA;       //串口1状态字
extern u16 USART2_RX_STA;       //串口2状态字
extern u16 fm_frame_index_byte;//FM广播01序列转为字节流的下标
extern u8 flag_is_wakeup_frame;//当前帧是唤醒帧的标志位
extern u8 wakeup_times;//唤醒帧发送的次数
u8 wakeup_times_index=0;//唤醒帧发送次数的索引值
u8 delay_index=0;//帧间隔延迟的索引下标
//定时器3中断服务程序
void TIM3_IRQHandler(void)   //TIM3中断
{
	if (TIM_GetITStatus(TIM3, TIM_IT_Update) != RESET)  //检查TIM3更新中断发生与否
	{	
//		if(t<fm_frame_index_bits){
//			PBout(6)=fm_frame_bits[t];//还需要加位同步，帧同步头
//			t++;
//		}
		if(flag_is_wakeup_frame==0){//非唤醒帧
			if(t<fm_frame_index_bits)//广播25+120*2；单播25+144*2；组播25+168*2；控制帧：25+84*2；
			{  			
				PAout(7)=fm_frame_bits[t];//frame_control[t];PBout(6)
				timer_67_stop=0;
				t++;
			}else//帧发送完毕后的清理
			{			
				TIM_Cmd(TIM3,DISABLE);
				PAout(7)=0;			
				t=0;
				timer_67_stop=1;
				delay_ms(10);//让FSK信号再振一会
				TIM_Cmd(TIM6,DISABLE);
				TIM_Cmd(TIM7,DISABLE);
				TIM_Cmd(TIM5, ENABLE); //使能TIMx
				USART_RX_STA=0;//处理完毕，允许接收下一帧
				usart1_works=0;//处理完毕，标志串口1允许使用
				usart2_works=0;//处理完毕，标志串口2允许使用
				
				fm_frame_index_byte=0;//字节流数组清空
				
				USART2_RX_STA=0;//处理完毕，允许接收下一帧。防止循环进入
				USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//打开中断
				USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//打开中断
			}//数据帧循环发送
		}
////////////////////////////////////////////////////////////////////////////////////////////////////

		if((flag_is_wakeup_frame==1)&&(wakeup_times_index<wakeup_times)){//唤醒帧
			if(t<fm_frame_index_bits)//广播25+120*2；单播25+144*2；组播25+168*2；控制帧：25+84*2；
			{  			
				PAout(7)=fm_frame_bits[t];//frame_control[t];
				timer_67_stop=0;
				t++;
			}else if(t>=fm_frame_index_bits){//此处要用else嵌套，否则信号电平不能维持一个码元时间
				PAout(7)=0;
				if(delay_index>=FRAME_INTERVAL){//发送完一帧唤醒帧，停顿一下，给终端留下处理时间
					wakeup_times_index++;//停顿完毕，表示一次唤醒帧完成，准备进入下一次
					delay_index=0;//索引归零
					t=0;
				}else{
					delay_index++;
				}
				if(delay_index==FSK_EXTEND)timer_67_stop=1;//FSK信号多震荡10ms，以免漏掉有用信号，然后将其关闭
				if((delay_index==FRAME_INTERVAL-FSK_EXTEND)&&(wakeup_times_index<(wakeup_times-1)))timer_67_stop=0;//FSK信号多震荡10ms，以免漏掉有用信号，将其提前打开  
								
			}
		}else if((flag_is_wakeup_frame==1)&&(wakeup_times_index>=wakeup_times)&&(wakeup_times!=0)){//多次发送完毕，清理
			flag_is_wakeup_frame=0;//多次唤醒帧发送完毕
			wakeup_times_index=0;
			wakeup_times=0;

			TIM_Cmd(TIM3,DISABLE);
			PAout(7)=0;			
//			t=0;
//			timer_67_stop=1;
//			delay_ms(10);//让FSK信号再振一会
			TIM_Cmd(TIM6,DISABLE);
			TIM_Cmd(TIM7,DISABLE);
			TIM_Cmd(TIM5, ENABLE); //使能TIMx
			USART_RX_STA=0;//处理完毕，允许接收下一帧
			usart1_works=0;//处理完毕，标志串口1允许使用
			usart2_works=0;//处理完毕，标志串口2允许使用
				
			fm_frame_index_byte=0;//字节流数组清空
			
			USART2_RX_STA=0;//处理完毕，允许接收下一帧。防止循环进入
			USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//打开中断
//			USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//打开中断
		}

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
u8 frame_send_buf_chip[15]={0};//串口2发送查询帧的缓冲区
u8 index_chipcheck_times=0;//芯片查询次数
extern u8 flag_safe_soc_ok;//安全芯片超时与应答。1：未应答；0：已应答；
u8 first_powerup=0;//记录首次上电。首次发送查询帧，总会漏掉第一个字符，不知为何
void TIM5_IRQHandler(void)   //TIM5中断
{
	u16 ttt=0;
	u8 index_frame_send_chip=0;//串口2连接帧下标
	u8 xor_sum=0;

	if (TIM_GetITStatus(TIM5, TIM_IT_Update) != RESET)  //检查TIM5更新中断发生与否
	{		
		LED1=!LED1;		
		if((secury_chip_ckeck>=CHIP_CHECK_FREQUENCY)&&(USART_RX_STA==0)&&(USART2_RX_STA==0)&&(usart1_works==0)&&((usart2_works==0)||(usart2_works==1))){//计数到5秒钟，当串口1、2都空闲时，开始查询，否则等待下一次中断时重新判断
			flag_safe_soc_ok=1;//查询前先标记不可用，串口2收到应答后再标记为可用
			LED0=0;//标记不可用
			usart2_works=1;//发送连接帧
			index_frame_send_chip=0;
			if(first_powerup==0){
			first_powerup=1;
				frame_send_buf_chip[index_frame_send_chip]=0;//前导
				index_frame_send_chip++;
			}
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
			xor_sum=XOR(frame_send_buf_chip,index_frame_send_chip);
			if((xor_sum==0x0D)||(xor_sum=='$'))xor_sum++;
			frame_send_buf_chip[index_frame_send_chip]=xor_sum;
			index_frame_send_chip++;
			frame_send_buf_chip[index_frame_send_chip]='\r';
			index_frame_send_chip++;
			frame_send_buf_chip[index_frame_send_chip]='\n';
			index_frame_send_chip++;
			for(ttt=0;ttt<index_frame_send_chip;ttt++)
			{
				USART_SendData(USART2, frame_send_buf_chip[ttt]);  //向串口2发送数据
				while(USART_GetFlagStatus(USART2,USART_FLAG_TC)!=SET);//等待发送结束
			}

			secury_chip_ckeck=0;		
		}else{
  			if(secury_chip_ckeck>50)secury_chip_ckeck=CHIP_CHECK_FREQUENCY;
			else  secury_chip_ckeck++;
			
		}
	}
	TIM_ClearITPendingBit(TIM5, TIM_IT_Update  );  //清除TIMx更新中断标志
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
			if(timer_67_stop==0)//用本标志位控制FSK信号的震动
				PBout(8)=~PBout(8);
			else GPIO_ResetBits(GPIOB,GPIO_Pin_8); //讨厌的蜂鸣器
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
			if(timer_67_stop==0)//用本标志位控制FSK信号的震动
				PBout(9)=~PBout(9);
			else GPIO_ResetBits(GPIOB,GPIO_Pin_9);
//				index_tim7++;
//				
//			}else//帧发送完毕后的清理
//			{
//				PBout(9)=0;
//				index_tim7=0;			
//			}//数据帧循环发送
				
		}
}

/***********************定时器4***************************/
void TIM4_Int_Init(u16 arr,u16 psc)
{
    TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
	NVIC_InitTypeDef NVIC_InitStructure;

	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE); //时钟使能
	
	//定时器TIM3初始化
	TIM_TimeBaseStructure.TIM_Period = arr; //设置在下一个更新事件装入活动的自动重装载寄存器周期的值	
	TIM_TimeBaseStructure.TIM_Prescaler =psc; //设置用来作为TIMx时钟频率除数的预分频值
	TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1; //设置时钟分割:TDTS = Tck_tim
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //TIM向上计数模式
	TIM_TimeBaseInit(TIM4, &TIM_TimeBaseStructure); //根据指定的参数初始化TIMx的时间基数单位
 
	TIM_ITConfig(TIM4,TIM_IT_Update,ENABLE ); //使能指定的TIM3中断,允许更新中断

	//中断优先级NVIC设置
	NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;  //TIM3中断
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 0;  //先占优先级0级
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 3;  //从优先级3级
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE; //IRQ通道被使能
	NVIC_Init(&NVIC_InitStructure);  //初始化NVIC寄存器


	TIM_Cmd(TIM4, DISABLE);  //关闭TIMx							 
}

extern u16 fm_frame_index_byte;//FM广播01序列字节流下标
//定时器4中断服务程序
void TIM4_IRQHandler(void)   //TIM4中断
{
   	if (TIM_GetITStatus(TIM4, TIM_IT_Update) != RESET)  //检查TIM4更新中断发生与否
		{
			TIM_ClearITPendingBit(TIM4, TIM_IT_Update);  //清除TIMx更新中断标志 
	//		PBout(9)=~PBout(9);//先做测试，看看定时器起来了没有
			if(flag_safe_soc_ok==1){//安全芯片应答超时
				LED0=0;//打开警示灯
				TIM_Cmd(TIM3,DISABLE);
			    PAout(7)=0;
				timer_67_stop=1;			  				
				TIM_Cmd(TIM6,DISABLE);
				TIM_Cmd(TIM7,DISABLE);
				TIM_Cmd(TIM5, ENABLE); //使能TIMx
				usart1_works=0;//处理完毕，标志串口1允许使用
				usart2_works=0;//处理完毕，标志串口2允许使用
				USART_RX_STA=0;
				USART2_RX_STA=0;
				fm_frame_index_byte=0;
				USART_ITConfig(USART1, USART_IT_RXNE, ENABLE);//打开中断
				USART_ITConfig(USART2, USART_IT_RXNE, ENABLE);//打开中断
			}
			TIM_Cmd(TIM4,DISABLE);//关闭超时判断定时器	
		}
}







