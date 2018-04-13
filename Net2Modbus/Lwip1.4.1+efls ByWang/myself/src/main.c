/**
  ******************************************************************************
  * @file    main.c
  * @author  MCD Application Team
  * @version V1.0.0
  * @date    11/20/2009
  * @brief   Main program body
  ******************************************************************************
  * @copy
  *
  * THE PRESENT FIRMWARE WHICH IS FOR GUIDANCE ONLY AIMS AT PROVIDING CUSTOMERS
  * WITH CODING INFORMATION REGARDING THEIR PRODUCTS IN ORDER FOR THEM TO SAVE
  * TIME. AS A RESULT, STMICROELECTRONICS SHALL NOT BE HELD LIABLE FOR ANY
  * DIRECT, INDIRECT OR CONSEQUENTIAL DAMAGES WITH RESPECT TO ANY CLAIMS ARISING
  * FROM THE CONTENT OF SUCH FIRMWARE AND/OR THE USE MADE BY CUSTOMERS OF THE
  * CODING INFORMATION CONTAINED HEREIN IN CONNECTION WITH THEIR PRODUCTS.
  *
  * <h2><center>&copy; COPYRIGHT 2009 STMicroelectronics</center></h2>
  */

/* Includes ------------------------------------------------------------------*/
#include "stm32f10x.h"
#include "stm32_eth.h"
#include "netconf.h"
#include "main.h"
#include "filesystem.h"
#include "ds18b20.h"
#include "flash_if.h"  


#ifdef USED_MODBUS
#include "stm32f10x_it.h"
#include "usart.h"
/* ----------------------- Modbus includes ----------------------------------*/
#include "mb.h"
#include "mbport.h"
#endif

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
#define SYSTEMTICK_PERIOD_MS  10

/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
__IO uint32_t LocalTime = 0; /* this variable is used to create a time reference incremented by 10ms */
uint32_t timingdelay;

/* Private function prototypes -----------------------------------------------*/
void System_Periodic_Handle(void);
void InitTIM(void);
void TIM2_IRQHandler(void);
/* Private functions ---------------------------------------------------------*/

/**
  * @brief  Main program.
  * @param  None
  * @retval None
  */

 extern void ModbusSenttset(uint32_t Time);
extern void UDPSendData(uint32_t Time);
extern void SendDataToSever(uint32_t Time);  
extern void tcp_sever_test(void);
extern void FileSystemInit(void);
extern void FileSystemThread(void); 
extern void httpd_init(void);
extern void initsi4432(void);
extern  void  Si4432Thread(uint32_t t);


char poll_net_data_lock=0;
uint32_t flash_net_buf[2];
uint8_t ipaddr_sever[4];
uint16_t port_sever;
int main(void)
{
  
	
		   {
			
				  //��ȡ����
	
		    GetIpFromFlash(flash_net_buf);//��ȡ2���ֵ�buf��
            ipaddr_sever[0]=flash_net_buf[0]>>24;//ȡ�����IPλ
			ipaddr_sever[1]=flash_net_buf[0]>>16;
			ipaddr_sever[2]=flash_net_buf[0]>>8;
			ipaddr_sever[3]=flash_net_buf[0]>>0;//ȡ�����IPλ
			port_sever     =flash_net_buf[1];



			 /*
			   //д����

			ipaddr_sever[0]=0x12;//ȡ�����IPλ
			ipaddr_sever[1]=0x34;
			ipaddr_sever[2]=0x56;
			ipaddr_sever[3]=0x78;

			  port_sever=2301;


			flash_net_buf[0]=((uint32_t)(ipaddr_sever[0]<<24))+
			                 ((uint32_t)(ipaddr_sever[1]<<16))+
							 ((uint32_t)(ipaddr_sever[2]<<8))+
							 ((uint32_t)(ipaddr_sever[3]<<0));
	
			flash_net_buf[1]= port_sever;
			
			WriteIpToFlash(flash_net_buf,2);

				 */

			}

	 /* Setup STM32 system (clocks, Ethernet, GPIO, NVIC) */
		  System_Setup();
		 
		  initsi4432();
		   rx_data();
		 	 
	      InitTIM();//systick ���� ��TIM���� systick ר����DS18B20��US��ʱ����
			 
	      #ifdef USED_FILESYSTEM
		   FileSystemInit(); 
	       FileSystemThread();    
		  #endif 
			
		     
		  /* Initilaize the LwIP satck */
		  LwIP_Init();

		 // netbios_init();
		 
		  //tftpd_init();

		  //tcp_sever_test();

		  #ifdef USED_FILESYSTEM && USED_HTTP
		  /*Infinite loop */
		  httpd_init();
		  #endif

		  #ifdef USED_SMTP
		  my_smtp_test("The Board is Power up...");
		  #endif

		   
		  while (1)
		  {   
		   	  #ifdef USED_DS18B20
			  TemperatureThread(LocalTime);
			  #endif
			  TcpTestThread(LocalTime);
		      SendDataToSever(LocalTime);
		    //  UDPSendData(LocalTime);
		      LwIP_Periodic_Handle(LocalTime);
			  #ifdef USED_MODBUS
			  ModbusSenttset(LocalTime); //����Ƿ���TCP���ܵ����ݣ�����з��� PASS RTU
			 (void)eMBMasterPoll();//���м�� modbus RTU ����
			  #endif

			  Si4432Thread(LocalTime);
	
		  }
}

/**
  * @brief  Inserts a delay time.
  * @param  nCount: number of 10ms periods to wait for.
  * @retval None
  */
void Delay(uint32_t nCount)
{
  /* Capture the current local time */
  timingdelay = LocalTime + nCount;  

  /* wait until the desired delay finish */  
  while(timingdelay > LocalTime)
  {     
  }
}

/**
  * @brief  Updates the system local time
  * @param  None
  * @retval None
  */
void Time_Update(void)
{
 // LocalTime += SYSTEMTICK_PERIOD_MS
}

 /*
		Systick ����DS18B20�ľ�ȷ��ʱ�ϡ�
		TIM2���ڱ��ص�ʱ�����
		20151020

  */
void TIM2_IRQHandler(void)
{
	
	if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
	{
		
		TIM_ClearFlag(TIM2, TIM_FLAG_Update);	     //���жϱ��
		TIM_ClearITPendingBit(TIM2, TIM_IT_Update);	 //�����ʱ��TIM2����жϱ�־λ

		LocalTime += SYSTEMTICK_PERIOD_MS;
  	
	}
	
}	


void InitTIM(void)
{	
    uint16_t 	usPrescalerValue;
	NVIC_InitTypeDef NVIC_InitStructure;
	TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
	//====================================ʱ�ӳ�ʼ��===========================
	//ʹ�ܶ�ʱ��2ʱ��
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
	//====================================��ʱ����ʼ��===========================
	//��ʱ��ʱ�������˵��
	//HCLKΪ72MHz��APB1����2��ƵΪ36MHz
	//TIM2��ʱ�ӱ�Ƶ��Ϊ72MHz��Ӳ���Զ���Ƶ,�ﵽ���
	//TIM2�ķ�Ƶϵ��Ϊ3599��ʱ���Ƶ��Ϊ72 / (1 + Prescaler) = 20KHz,��׼Ϊ50us
	//TIM������ֵΪusTim1Timerout50u	
	usPrescalerValue = (uint16_t) (72000000 / 20000) - 1;
	//Ԥװ��ʹ��
	TIM_ARRPreloadConfig(TIM2, ENABLE);
	//====================================�жϳ�ʼ��===========================
	//����NVIC���ȼ�����ΪGroup2��0-3��ռʽ���ȼ���0-3����Ӧʽ���ȼ�
	NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);
	NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 3;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;
	NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
	NVIC_Init(&NVIC_InitStructure);
	//�������жϱ�־λ
	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	//��ʱ��2����жϹر�
	TIM_ITConfig(TIM2, TIM_IT_Update, DISABLE);
	//��ʱ��3����
	TIM_Cmd(TIM2, DISABLE);

	TIM_TimeBaseStructure.TIM_Prescaler = usPrescalerValue;
	TIM_TimeBaseStructure.TIM_ClockDivision = 0;
	TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseStructure.TIM_Period = (uint16_t)(10 * 1000 / 50);//10ms
	TIM_TimeBaseInit(TIM2, &TIM_TimeBaseStructure);

	TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
	TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);
	TIM_SetCounter(TIM2, 0);
	TIM_Cmd(TIM2, ENABLE);	


}



#ifdef  USE_FULL_ASSERT

/**
  * @brief  Reports the name of the source file and the source line number
  *   where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t* file, uint32_t line)
{
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

  /* Infinite loop */
  while (1)
  {}
}
#endif


 





/******************* (C) COPYRIGHT 2009 STMicroelectronics *****END OF FILE****/