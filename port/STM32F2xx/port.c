/*
 * port_smonitor.c
 *
 *  Created on: 
 *      Author: 
 */

#ifdef USE_SYS_MONITOR

/*============================ INCLUDES ======================================*/
#include <stdint.h>
#include "board.h"
#include "stm32f2xx_conf.h"

/*============================ PRIVATE DEFINITIONS ===========================*/


/*============================ TYPES =========================================*/


/*============================ VARIABLES =====================================*/
extern char SMBuff[SYS_MONITOR_BUFF_SIZE];


/*============================ PRIVATE PROTOTYPES ============================*/


/*============================ IMPLEMENTATION (PRIVATE FUNCTIONS) ============*/


/*============================ IMPLEMENTATION (PUBLIC FUNCTIONS) =============*/

void portSysMonitor_Init(void){
GPIO_InitTypeDef GPIO_InitStruct;
USART_InitTypeDef USART_InitStruct;
DMA_InitTypeDef DMA_InitStruct;

	// RCC Peripheral Config:
	#ifdef SM_RCC_AHB1Peripheral
		RCC_AHB1PeriphClockCmd(SM_RCC_AHB1Peripheral, ENABLE);
	#endif
	#ifdef SM_RCC_AHB2Peripheral
		RCC_AHB2PeriphClockCmd(SM_RCC_AHB2Peripheral, ENABLE);
	#endif
	#ifdef SM_RCC_AHB3Peripheral
		RCC_AHB3PeriphClockCmd(SM_RCC_AHB3Peripheral, ENABLE);
	#endif
	#ifdef SM_RCC_APB1Peripheral
		RCC_APB1PeriphClockCmd(SM_RCC_APB1Peripheral, ENABLE);
	#endif
	#ifdef SM_RCC_APB2Peripheral
		RCC_APB2PeriphClockCmd(SM_RCC_APB2Peripheral , ENABLE);
	#endif

	// PIN RX:
	GPIO_InitStruct.GPIO_Pin = SM_RX_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(SM_RX_PORT, &GPIO_InitStruct);
	GPIO_PinAFConfig(SM_RX_PORT, SM_RX_PinSource, SM_GPIO_AF);

	// PIN TX:
	GPIO_InitStruct.GPIO_Pin = SM_TX_PIN;
	GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF;
	//GPIO_InitStruct.GPIO_Mode = GPIO_Mode_OUT;
	GPIO_InitStruct.GPIO_Speed = GPIO_Speed_100MHz;
	GPIO_InitStruct.GPIO_OType = GPIO_OType_PP;
	GPIO_InitStruct.GPIO_PuPd = GPIO_PuPd_UP;
	GPIO_Init(SM_TX_PORT, &GPIO_InitStruct);
	GPIO_PinAFConfig(SM_TX_PORT, SM_TX_PinSource, SM_GPIO_AF);

	// DMA:
	DMA_InitStruct.DMA_PeripheralBaseAddr = (uint32_t)(&SM_USART->DR);
	DMA_InitStruct.DMA_Memory0BaseAddr = (uint32_t)SMBuff;
	DMA_InitStruct.DMA_DIR = DMA_DIR_MemoryToPeripheral;
	DMA_InitStruct.DMA_BufferSize = SYS_MONITOR_BUFF_SIZE;
	DMA_InitStruct.DMA_PeripheralInc = DMA_PeripheralInc_Disable;
	DMA_InitStruct.DMA_MemoryInc = DMA_MemoryInc_Enable;
	DMA_InitStruct.DMA_PeripheralDataSize = DMA_PeripheralDataSize_Byte;
	DMA_InitStruct.DMA_MemoryDataSize = DMA_MemoryDataSize_Byte;
	DMA_InitStruct.DMA_Mode = DMA_Mode_Normal;
	DMA_InitStruct.DMA_Priority = DMA_Priority_Medium;
	DMA_InitStruct.DMA_Channel = SM_DMA_Channel;
	DMA_InitStruct.DMA_FIFOMode = DMA_FIFOMode_Disable;
	DMA_InitStruct.DMA_FIFOThreshold = DMA_FIFOThreshold_HalfFull;
	DMA_InitStruct.DMA_MemoryBurst = DMA_MemoryBurst_Single;
	DMA_InitStruct.DMA_PeripheralBurst = DMA_PeripheralBurst_Single;
	DMA_Init(SM_DMA_Stream, &DMA_InitStruct);

	// USART:
	USART_InitStruct.USART_BaudRate = SM_BAUDRATE;
	USART_InitStruct.USART_WordLength = USART_WordLength_8b;
	USART_InitStruct.USART_StopBits = USART_StopBits_1;
	USART_InitStruct.USART_Parity = USART_Parity_No;
	USART_InitStruct.USART_Mode = USART_Mode_Rx | USART_Mode_Tx;
	USART_InitStruct.USART_HardwareFlowControl = USART_HardwareFlowControl_None;
	USART_Init(SM_USART, &USART_InitStruct);
	//--
	USART_DMACmd(SM_USART, USART_DMAReq_Tx, ENABLE);
	USART_Cmd(SM_USART, ENABLE);
}//portSysMonitor_Init

void portSysMonitor_TxBuff(uint16_t lenght){
	DMA_ClearFlag(SM_DMA_Stream, SM_DMA_FLAG_TC);
	SM_DMA_Stream->NDTR = lenght;
	SM_DMA_Stream->M0AR = (uint32_t)SMBuff;
	DMA_Cmd(SM_DMA_Stream, ENABLE);
}//portSysMonitor_TxBuff

void portSysMonitor_CONFIGURE_TIMER_FOR_RUN_TIME_STATS(void) {
TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
	/* Time Base configuration */
	TIM_TimeBaseStruct.TIM_Period = 0xFFFFFFFF;
	TIM_TimeBaseStruct.TIM_Prescaler = 60;			// 1us
	TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
	TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
	TIM_TimeBaseInit(SM_TC, &TIM_TimeBaseStruct);
	TIM_Cmd(SM_TC, ENABLE);
}//portSysMonitor_CONFIGURE_TIMER_FOR_RUN_TIME_STATS

uint32_t portSysMonitor_GetRunTimeCounterValue(void){
	return TIM_GetCounter(SM_TC);
}//portSysMonitor_GetRunTimeCounterValue

#endif /* USE_SYS_MONITOR */
