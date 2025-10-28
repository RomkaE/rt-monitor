
/*============================ INCLUDES ======================================*/

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "sys_monitor_cfg.h"
#include "../inc/port.h"

#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"

/*============================ PRIVATE DEFINITIONS ===========================*/


/*============================ TYPES =========================================*/


/*============================ VARIABLES =====================================*/

static const uint8_t *s_pBufUART;
static uint16_t s_sizeBufUART;
static uint16_t s_idxBufUART;

/*============================ PRIVATE PROTOTYPES ============================*/


/*============================ IMPLEMENTATION (PRIVATE FUNCTIONS) ============*/

ISR(USART1_UDRE_vect)
{
  UDR1 = s_pBufUART[s_idxBufUART];
  s_idxBufUART++;
  if (s_idxBufUART >= s_sizeBufUART)
  {
    UCSR1B &= ~(1 << UDRIE1);   // DISABLE <Data Register Empty Interrupt>
    s_idxBufUART = 0;
//    osal_semaphore_post(s_SemUart, true);
  }
}

/*============================ IMPLEMENTATION (PUBLIC FUNCTIONS) =============*/

void portSysMonitor_Init(void)
{
  UBRR1H = 0;
  UBRR1L = 8;           // 115200
  UCSR1B |= (1<<TXEN1);

  UCSR1C |= (1<<UCSZ11) | (1<<UCSZ10);         // 8-bit frame

  // Очистить флаги:
  UDR1;                   // dummy read
  UCSR1A |= (1 << TXC1) | (1 << RXC1); // сбросить TXC/RXC
}

void portSysMonitor_TxBuff(const void *_buff, uint16_t _lenght)
{
  s_sizeBufUART = _lenght;
  s_pBufUART = _buff;
  UCSR1B |= (1<<UDRIE1);    // ENABLE <Data Register Empty Interrupt>
}

void portSysMonitor_CONFIGURE_TIMER_FOR_RUN_TIME_STATS(void)
{
  TCNT3 = 0;      // clear counter
  TCCR3A = 0;     // normal mode
  TIFR3 = 0xFF;   // clear flags

  // CS | DIV
  //  1 |  1
  //  2 |  8
  //  3 |  64
  //  4 |  256
  //  5 |  1024
  TCCR3B = (4 << CS30);   // div 256
}

configRUN_TIME_COUNTER_TYPE portSysMonitor_GetRunTimeCounterValue(void)
{
  configRUN_TIME_COUNTER_TYPE ret;

  #if (configRUN_TIME_TYPE_WIDTH == TICK_TYPE_WIDTH_16_BITS)
    ret = TCNT3;
  #elif (configRUN_TIME_TYPE_WIDTH == TICK_TYPE_WIDTH_32_BITS)
    static uint32_t counter = 0;
    static uint16_t prev = 0;
    uint16_t curr = TCNT3;
    counter += (uint16_t)(curr - prev);
    prev = curr;
    ret = counter;
  #else
    #error "Unsupported configRUN_TIME_COUNTER_TYPE size"
  #endif

  return ret;
}
