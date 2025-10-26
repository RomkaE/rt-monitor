
/*============================ INCLUDES ======================================*/

#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "sys_monitor_cfg.h"
#include "../inc/port.h"

#include "FreeRTOSConfig.h"

/*============================ PRIVATE DEFINITIONS ===========================*/


/*============================ TYPES =========================================*/


/*============================ VARIABLES =====================================*/

static uint8_t *s_pBufUART;
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
  // Сбросить счётчик
  TCNT3 = 0;

  // Настроить Timer3 в нормальный режим (счёт до переполнения)
  TCCR3A = 0;

  // Запустить таймер с делителем 256: (CS32 = 1, CS31 = 0, CS30 = 0)
  TCCR3B = (1 << CS32);

  // Можно очистить флаги, если нужно:
  TIFR3 = 0xFF;
}

configRUN_TIME_COUNTER_TYPE portSysMonitor_GetRunTimeCounterValue(void)
{
  uint16_t tim_cnt = TCNT3;
  return (configRUN_TIME_COUNTER_TYPE)tim_cnt;
}
