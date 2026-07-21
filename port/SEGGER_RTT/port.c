
#include <stdbool.h>
#include <stdint.h>
#include "rtmon_config.h"
#include "../port.h"

#include "app-error/app_assert.h"

// FreeRTOS:
#include "FreeRTOS.h"
#include "task.h"

#if defined(RTMON_RTT_BUFF_IDX)

#include "SEGGER_RTT.h"

#ifndef RTMON_CFG_XMIT_RETRIES
  #define RTMON_CFG_XMIT_RETRIES    ( 2 )
#endif
#ifndef RTMON_CFG_XMIT_RETRY_MS
  #define RTMON_CFG_XMIT_RETRY_MS   ( 10 )
#endif

static const uint8_t s_TermSwitch[2] = { 0xFFu, (uint8_t)('0' + RTMON_RTT_TERMINAL) };

void rtmon_portInit(void)
{
  SEGGER_RTT_Init();
}

void rtmon_xmitBuf(const char *_buf, const size_t _lenght)
{
  size_t buf_size = _lenght;
  unsigned retries = RTMON_CFG_XMIT_RETRIES;

  while (buf_size)
  {
    uint32_t sent_n = 0;

    SEGGER_RTT_LOCK();
    {
      unsigned avail = SEGGER_RTT_GetAvailWriteSpace(RTMON_RTT_BUFF_IDX);
      if (avail > sizeof(s_TermSwitch))
      {
        uint32_t send_n = avail - sizeof(s_TermSwitch);
        if (send_n > buf_size)
          send_n = buf_size;
        SEGGER_RTT_WriteNoLock(RTMON_RTT_BUFF_IDX, s_TermSwitch, sizeof(s_TermSwitch));
        sent_n = SEGGER_RTT_WriteNoLock(RTMON_RTT_BUFF_IDX, _buf, send_n);
      }
    }
    SEGGER_RTT_UNLOCK();

    if (sent_n)
    {
      // Buffer iteration:
      _buf += sent_n;
      buf_size -= sent_n;

      // Reset retries counter:
      retries = RTMON_CFG_XMIT_RETRIES;
    }
    else if (retries--)
      vTaskDelay(pdMS_TO_TICKS(RTMON_CFG_XMIT_RETRY_MS));   // wait for the next attempt
    else
      return;   // attempts exhausted - drop data and exit
  }
}

#endif /* defined(RTMON_RTT_BUFF_IDX) */
