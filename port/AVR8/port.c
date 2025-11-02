
#include <assert.h>
#include <stdint.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#include "../port.h"
#include "rtmon_config.h"

// FreeRTOS:
#include "FreeRTOS.h"
#include "semphr.h"

static const char *s_pBufUART;
static size_t s_sizeBufUART, s_idxBufUART;

static SemaphoreHandle_t s_SemXmitHandle;
static StaticSemaphore_t s_SemXmit;

ISR(USART1_UDRE_vect)
{
  UDR1 = s_pBufUART[s_idxBufUART];
  s_idxBufUART++;
  if (s_idxBufUART >= s_sizeBufUART)
  {
    UCSR1B &= ~(1 << UDRIE1);   // DISABLE <Data Register Empty Interrupt>

    BaseType_t switch_context = pdFALSE;
    xSemaphoreGiveFromISR(s_SemXmitHandle, switch_context);
    if (switch_context != pdFALSE)
    {
//      portEND_SWITCHING_ISR();    // TODO
    }
  }
}

void rtmon_portInit(void)
{
  UBRR1H = 0;
  UBRR1L = 8;           // 115200
  UCSR1B |= (1<<TXEN1);

  UCSR1C |= (1<<UCSZ11) | (1<<UCSZ10);         // 8-bit frame

  // Очистить флаги:
  UDR1;                   // dummy read
  UCSR1A |= (1 << TXC1) | (1 << RXC1); // сбросить TXC/RXC

  s_SemXmitHandle = xSemaphoreCreateBinaryStatic(&s_SemXmit);
  assert(s_SemXmitHandle != NULL);
  xSemaphoreTake(s_SemXmitHandle, 0);
}

void rtmon_xmitBuf(const char *_buf, const size_t _lenght)
{
  // Wait for the last operation to complete:
  xSemaphoreTake(s_SemXmitHandle, portMAX_DELAY);

  // Send:
  s_sizeBufUART = _lenght;
  s_pBufUART = _buf;
  s_idxBufUART = 0;
  UCSR1B |= (1<<UDRIE1);    // ENABLE <Data Register Empty Interrupt>
}

void rtmon_portInitRunTimer(void)
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

configRUN_TIME_COUNTER_TYPE rtmon_portGetRunTimer(void)
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
