
#include <assert.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "../port.h"
#include "rtmon_config.h"

#if RTMON_ENABLED

#ifndef RTMON_PORT_XMIT_EXTERNAL
  #define RTMON_PORT_XMIT_EXTERNAL    0
#endif

// FreeRTOS:
#include "FreeRTOS.h"
#include "semphr.h"

#if !RTMON_PORT_XMIT_EXTERNAL
static const char *s_pBufUART;
static size_t s_sizeBufUART, s_idxBufUART;

static SemaphoreHandle_t s_SemXmitHandle;
static StaticSemaphore_t s_SemXmit;
#endif /* !RTMON_PORT_XMIT_EXTERNAL */

// Counts Timer3 overflows so the run-time counter stays correct regardless
// of how long a caller goes between rtmon_portGetRunTimer() calls (unlike
// polling TCNT3's delta, which only survives a single wrap):
static volatile uint16_t s_Ovf3Count = 0;

ISR(TIMER3_OVF_vect)
{
  s_Ovf3Count++;
}

#if !RTMON_PORT_XMIT_EXTERNAL
ISR(USART0_UDRE_vect)
{
  UDR0 = s_pBufUART[s_idxBufUART];
  s_idxBufUART++;
  if (s_idxBufUART >= s_sizeBufUART)
  {
    UCSR0B &= ~(1 << UDRIE0);   // DISABLE <Data Register Empty Interrupt>

    BaseType_t switch_context = pdFALSE;
    xSemaphoreGiveFromISR(s_SemXmitHandle, &switch_context);
    if (switch_context != pdFALSE)
      portYIELD_FROM_ISR();
  }
}
#endif /* !RTMON_PORT_XMIT_EXTERNAL */

void rtmon_portInit(void)
{
#if !RTMON_PORT_XMIT_EXTERNAL
  UBRR0H = 0;
  UBRR0L = 8;           // 115200
  UCSR0B |= (1<<TXEN0);

  UCSR0C |= (1<<UCSZ01) | (1<<UCSZ00);         // 8-bit frame

  // Очистить флаги:
  UDR0;                   // dummy read
  UCSR0A |= (1 << TXC0) | (1 << RXC0); // сбросить TXC/RXC

  s_SemXmitHandle = xSemaphoreCreateBinaryStatic(&s_SemXmit);
  assert(s_SemXmitHandle != NULL);
#endif /* !RTMON_PORT_XMIT_EXTERNAL */
}

#if !RTMON_PORT_XMIT_EXTERNAL
void rtmon_xmitBuf(const char *_buf, const size_t _lenght)
{
  // Send:
  s_sizeBufUART = _lenght;
  s_pBufUART = _buf;
  s_idxBufUART = 0;
  UCSR0B |= (1<<UDRIE0);    // ENABLE <Data Register Empty Interrupt>

  // Wait for the operation to complete:
  xSemaphoreTake(s_SemXmitHandle, portMAX_DELAY);

}
#endif /* !RTMON_PORT_XMIT_EXTERNAL */

void rtmon_portInitRunTimer(void)
{
  TCNT3 = 0;      // clear counter
  TCCR3A = 0;     // normal mode
  TIFR3 = 0xFF;   // clear flags
  s_Ovf3Count = 0;

  // CS | DIV
  //  1 |  1
  //  2 |  8
  //  3 |  64
  //  4 |  256
  //  5 |  1024
  TCCR3B = (2 << CS30);   // div 8

  TIMSK3 |= (1 << TOIE3);  // enable Timer3 overflow interrupt
}

configRUN_TIME_COUNTER_TYPE rtmon_portGetRunTimer(void)
{
  configRUN_TIME_COUNTER_TYPE ret;

  #if (configRUN_TIME_TYPE_WIDTH == TICK_TYPE_WIDTH_16_BITS)
    ret = TCNT3;
  #elif (configRUN_TIME_TYPE_WIDTH == TICK_TYPE_WIDTH_32_BITS)
    uint8_t sreg = SREG;
    cli();
    uint16_t tcnt = TCNT3;
    uint16_t ovf = s_Ovf3Count;
    // A wrap may have happened right before this read but after the ISR last
    // ran (we're inside a critical section, so TIMER3_OVF_vect can't have
    // fired for it yet) - catch it via the pending overflow flag:
    if ((TIFR3 & (1 << TOV3)) && tcnt < 0x8000)
      ovf++;
    SREG = sreg;

    ret = ((uint32_t)ovf << 16) | tcnt;
  #else
    #error "Unsupported configRUN_TIME_COUNTER_TYPE size"
  #endif

  return ret;
}

#endif
