
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

ISR(USART0_UDRE_vect)
{
  UDR0 = s_pBufUART[s_idxBufUART];
  s_idxBufUART++;
  if (s_idxBufUART >= s_sizeBufUART)
  {
    UCSR0B &= ~(1 << UDRIE0);   // DISABLE <Data Register Empty Interrupt>

    BaseType_t switch_context = pdFALSE;
    xSemaphoreGiveFromISR(s_SemXmitHandle, switch_context);
    if (switch_context != pdFALSE)
      portYIELD_FROM_ISR();
  }
}

void rtmon_portInit(void)
{
  UBRR0H = 0;
  UBRR0L = 8;           // 115200
  UCSR0B |= (1<<TXEN0);

  UCSR0C |= (1<<UCSZ01) | (1<<UCSZ00);         // 8-bit frame

  // Очистить флаги:
  UDR0;                   // dummy read
  UCSR0A |= (1 << TXC0) | (1 << RXC0); // сбросить TXC/RXC

  s_SemXmitHandle = xSemaphoreCreateBinaryStatic(&s_SemXmit);
  assert(s_SemXmitHandle != NULL);
}

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
