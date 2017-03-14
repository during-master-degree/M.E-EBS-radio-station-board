#include "gray.h"

void gray_encode(
		unsigned char bitin[],      /*待编码的数据*/
		unsigned char code[]		/*编码后的数据有这样的关系 code[ 24]={bitin[11]--bitin[0],Reg[10],Reg[0],parity} 构成*/
			)      
{	
	unsigned char Reg[11]={0};             /*11个寄存器初始化为0*/
	unsigned char i=0,j=0,sum=0;                        /*循环控制变量*/
	unsigned char charer_value[12];			/*中间变量节点*/
	
	/*=============根据生成系统码的乘除法电路得到以下关系====================================================*/
	for(i=0;i<M;i++)
	{	
		/*计算中间变量节点的数据*/
		charer_value[11]=(bitin[M-i-1]+Reg[10])%2;  /*输入的数据是低位先输入，即bitin[0]表示m0*/
		charer_value[10]=charer_value[11];          /*charer_value[11]是由当前输入的数据位和寄存器Reg[10]异或得到*/
		charer_value[6]=charer_value[10];
		charer_value[5]=charer_value[6];
		charer_value[4]=charer_value[5];
		charer_value[2]=charer_value[4];
		charer_value[0]=charer_value[2];
		/*计算寄存器节点的数据*/
		Reg[10]=(Reg[9]+charer_value[10])%2;           /*Reg[4]是由Reg[3]的数据和charer_value[4]异或得到*/
		Reg[9]=Reg[8];                                        /*Reg[3]是由Reg[2]的数据更新得到*/
		Reg[8]=Reg[7];
		Reg[7]=Reg[6];
		Reg[6]=(Reg[5]+charer_value[6])%2;          /*Reg[2]是由Reg[1]的数据和charer_value[2]异或后更新得到*/
		Reg[5]=(Reg[4]+charer_value[5])%2;
		Reg[4]=(Reg[3]+charer_value[4])%2;
		Reg[3]=Reg[2];											/*Reg[1]是由Reg[0]的数据更新得到*/
		Reg[2]=(Reg[1]+charer_value[2])%2;
		Reg[1]=Reg[0];	
		Reg[0]=charer_value[0];								/*Reg[0]是由charer_value[0]的数据更新得到*/
		/*计算当前输入数据的编码结果*/
		code[i]=bitin[M-i-1];                     /*每个码字的信息位直接由输入的信息构成*/
		
	}
	/*=====================后M（R-1）次循环得到码字的校验位========================*/
	for(j=0;j<11;j++)
	{	
		code[i]=Reg[10-j];			/*校验位是由最后的Reg[10]--Reg[0]构成*/
		i++;
	}
	for(i=0;i<23;i++)
		sum=sum+code[i];
	code[23]=sum%2;
	
	return;
}


u8 decode_error_catch(u8 r1_code[24],u8 de_out[24]) /*修正的捕错译码*/
{
	char de_reg[11]={0};
	char r_code[23]={0};
//	char T=0;
	char middle=0;
	char flag=0,sum=0;
	char correct_bit=0;
	char charer[11]={0};
	char weight=0;
	char i=0,k=0;
	int j=0;

/*====================提取码字的前23位，得到（23，12）完备格雷码===========================*/
	
	for(i=0;i<23;i++)
		r_code[i]=r1_code[i];

/*===================将23位码字依次送入伴随式寄存器=====================================*/
	
	 for(k=0;k<23;k++)
	{  middle=de_reg[10];                 /*输入11位后，再输入时，寄存器经反馈更新，反馈值为middle*/

	   de_reg[10]=(middle+de_reg[9])%2;
       de_reg[9]=de_reg[8];
	   de_reg[8]=de_reg[7];
	   de_reg[7]=de_reg[6];
	   de_reg[6]=(de_reg[5]+middle)%2;
	   de_reg[5]=(de_reg[4]+middle)%2;
	   de_reg[4]=(de_reg[3]+middle)%2;
	   de_reg[3]=de_reg[2];
	   de_reg[2]=(de_reg[1]+middle)%2;
	   de_reg[1]=de_reg[0];
	   de_reg[0]=(r_code[k]+middle)%2;
  }                                

/*=================计算伴随式的重量===============================*/
 f1: for(i=0;i<11;i++)
    weight=weight+de_reg[i]; 

/*==========根据伴随式的重量大小，进行纠错译码======================*/
  if(weight<=3)
	{//  T=1;                 /*错误均在后11位伴随式的值即为错误图样*/
  for(i=0;i<11;i++)
	  charer[i]=de_reg[i];          /*charer值即为错误图样*/
  
  }

  else 
  {   weight=0;
	                               /*Q(x)=x^5*/
	  charer[10]=de_reg[10];
	  charer[9]=(de_reg[9]+1)%2;            /*charer值即为S0-Si后得到的伴随式的值*/
	  charer[8]=(de_reg[8]+1)%2; 
      charer[7]=de_reg[7];
      charer[6]=(de_reg[6]+1)%2;
      charer[5]=(de_reg[5]+1)%2;
      charer[4]=de_reg[4];
      charer[3]=de_reg[3];
      charer[2]=(de_reg[2]+1)%2;
      charer[1]=(de_reg[1]+1)%2;
      charer[0]=de_reg[0];

	  for(i=0;i<11;i++)
		  weight=weight+charer[i];

	  if (weight<=2)
		{//  T=1;
	  r_code[(6+flag)%23]=(r_code[(6+flag)%23]+1)%2;}          /*信息位x^16纠错*/
	  else
	  {   
		  weight=0;                           /*Q(x)=x^6*/
		  charer[10]=(de_reg[10]+1)%2;
		  charer[9]=(de_reg[9]+1)%2;            /*charer值即为S0-Si后得到的伴随式的值，后11位的错误图样*/
		  charer[8]=de_reg[8];
          charer[7]=(de_reg[7]+1)%2;
		  charer[6]=(de_reg[6]+1)%2;
		  charer[5]=de_reg[5];
		  charer[4]=de_reg[4];
		  charer[3]=(de_reg[3]+1)%2;
		  charer[2]=(de_reg[2]+1)%2;
		  charer[1]=de_reg[1];
		  charer[0]=de_reg[0];
		  
		  for(i=0;i<11;i++)
		  weight=weight+charer[i];

		  if(weight<=2)
			{ // T=1;
		  r_code[(5+flag)%23]=(r_code[(5+flag)%23]+1)%2;           /*信息位x^17纠错*/
		  }
		  else 
			
			{  weight=0;
			   middle=de_reg[10];    /*以上3中情况均不成立，伴随式和缓存循环移位一次*/

	   de_reg[10]=(middle+de_reg[9])%2;
       de_reg[9]=de_reg[8];
	   de_reg[8]=de_reg[7];
	   de_reg[7]=de_reg[6];
	   de_reg[6]=(de_reg[5]+middle)%2;
	   de_reg[5]=(de_reg[4]+middle)%2;
	   de_reg[4]=(de_reg[3]+middle)%2;
	   de_reg[3]=de_reg[2];
	   de_reg[2]=(de_reg[1]+middle)%2;
	   de_reg[1]=de_reg[0];                  
	   de_reg[0]=middle;                       /*进行无输入的循环移位，计算伴随式*/
	                                     /*由于（23，12）格雷码是完备码，最多移位23次，即可完成纠错，错误数大于3，则发生错译*/
		flag=flag+1;             /*记录循环移位的次数*/
	  goto f1;
		  }
	  }
  }
  correct_bit=(12+flag)%23;                  /*开始纠错的位置，11级缓存的第一位输出*/

/*============================纠错过程==================================*/
  for(i=correct_bit,j=10;j>=0;i++,j--)              /*移位后相应的伴随式即为错误图样，r+e即完成到译码*/
 {
	 r_code[i%23]=(r_code[i%23]+charer[j])%2;
	   
  }
  for(i=0;i<23;i++)
  {
	  de_out[i]=r_code[i];
     sum=sum+de_out[i];
  }

  de_out[23]=sum%2;
/*=================完成一次译码后要消除伴随式的影响===============================*/
  for(i=0;i<11;i++)
  {
  de_reg[i]=0;
  
  }
  return 0;
}

