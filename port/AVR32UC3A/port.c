/*
 * smonitor.c
 *
 *  Created on:
 *      Author:
 */

/*============================ INCLUDES ======================================*/
#include <stdint.h>
#include "board.h"

#include <avr32/io.h>
#include "gpio.h"
#include "pdca.h"
#include "usart.h"
#include "tc.h"

#ifdef USE_SYS_MONITOR

/*============================ PRIVATE DEFINITIONS ===========================*/


/*============================ TYPES =========================================*/


/*============================ VARIABLES =====================================*/
extern char SMBuff[SYS_MONITOR_BUFF_SIZE];

/*============================ PRIVATE PROTOTYPES ============================*/


/*============================ IMPLEMENTATION (PRIVATE FUNCTIONS) ============*/


/*============================ IMPLEMENTATION (PUBLIC FUNCTIONS) =============*/

void portSysMonitor_Init(void){
gpio_map_t USART_GPIO_MAP = { { SM_TX_PIN, SM_TX_FUNCTION } };
usart_options_t USART_opt = {	.baudrate = SM_BAUDRATE,
								.charlength = 8,
								.paritytype = USART_NO_PARITY,
								.stopbits = USART_1_STOPBIT,
								.channelmode = USART_NORMAL_CHMODE };
pdca_channel_options_t PDCA_opt = {	.addr = &SMBuff[0],           									// memory address
									.pid = SM_DMA_PID,			           				// select peripheral - data are transmit on USART TX line.
									.size = SYS_MONITOR_BUFF_SIZE,										// transfer counter
									.r_addr = NULL,                           							// next memory address
									.r_size = 0,                              							// next transfer counter
									.transfer_size = PDCA_TRANSFER_SIZE_BYTE };

	gpio_enable_module( USART_GPIO_MAP, sizeof(USART_GPIO_MAP)/sizeof(USART_GPIO_MAP[0]) );
	usart_init_rs232( SM_USART, &USART_opt, PBA_HZ );
	pdca_init_channel(SM_DMA_Channel, &PDCA_opt);
}//portSysMonitor_Init

void portSysMonitor_TxBuff(uint16_t lenght){
	pdca_disable(SM_DMA_Channel);
	pdca_load_channel(SM_DMA_Channel, SMBuff, lenght);
	pdca_enable(SM_DMA_Channel);
}//portSysMonitor_TxBuff

void portSysMonitor_CONFIGURE_TIMER_FOR_RUN_TIME_STATS(void) {
tc_waveform_opt_t WAVEFORM_OPT = {
			.channel  = SM_TC_Channel,	             			     				// Channel selection.
			.bswtrg   = TC_EVT_EFFECT_NOOP,               		 				// Software trigger effect on TIOB.
			.beevt    = TC_EVT_EFFECT_NOOP,              		 				// External event effect on TIOB.
			.bcpc     = TC_EVT_EFFECT_NOOP,                		 				// RC compare effect on TIOB.
			.bcpb     = TC_EVT_EFFECT_NOOP,               		 				// RB compare effect on TIOB.
			.aswtrg   = TC_EVT_EFFECT_NOOP,                      				// Software trigger effect on TIOA.
			.aeevt    = TC_EVT_EFFECT_NOOP,                      				// External event effect on TIOA.
			.acpc     = TC_EVT_EFFECT_NOOP,                      				// RC compare effect on TIOA: toggle.
			.acpa     = TC_EVT_EFFECT_NOOP,                      				// RA compare effect on TIOA: toggle
																				// (other possibilities are none, set and clear).
			.wavsel   = TC_WAVEFORM_SEL_UP_MODE,
			.enetrg   = FALSE,                             		 				// External event trigger enable.
			.eevt     = 0,                                       				// External event selection.
			.eevtedg  = TC_SEL_NO_EDGE,                          				// External event edge selection.
			.cpcdis   = FALSE,                                   				// Counter disable when RC compare.
			.cpcstop  = FALSE,                                   				// Counter clock stopped with RC compare.
			.burst    = FALSE,                                   				// Burst signal selection.
			.clki     = FALSE,                                   				// Clock inversion.
			.tcclks   = TC_CLOCK_SOURCE_TC5                      				// Internal source clock 5 (fPBA / 128).
		};

		tc_init_waveform(SM_TC, &WAVEFORM_OPT);
		tc_start(SM_TC, SM_TC_Channel);
}//portSysMonitor_CONFIGURE_TIMER_FOR_RUN_TIME_STATS

uint32_t portSysMonitor_GetRunTimeCounterValue(void){
static uint32_t cnt = 0;
static uint16_t prevTC = 0;
uint16_t TC;
uint32_t dTC;

	TC = tc_read_tc(SM_TC, SM_TC_Channel);

	if (prevTC > TC)
		dTC = TC + (0xFFFF - prevTC);
	else
		dTC = TC - prevTC;

	prevTC = TC;
	cnt = cnt + dTC;

	return cnt;

}//portSysMonitor_GetRunTimeCounterValue

#endif /* USE_SYS_MONITOR_USE */
