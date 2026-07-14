
#include <stdbool.h>
#include <stdint.h>
#include "rtmon_config.h"
#include "../port.h"

#include "app-error/app_assert.h"
#include "tusb.h"

#if defined(RTMON_RTT_BUFF_IDX)

#include "SEGGER_RTT.h"

static osal_semaphore_def_t sem_def;
static osal_semaphore_t sem_cdc;

static const uint8_t s_TermSwitch[2] = { 0xFFu, (uint8_t)('0' + RTMON_RTT_TERMINAL) };

void rtmon_portInit(void)
{
  sem_cdc = osal_semaphore_create(&sem_def);
  ASSERT(sem_cdc != NULL);

  SEGGER_RTT_Init();
}

void rtmon_xmitBuf(const char *_buf, const size_t _lenght)
{
  size_t buf_size = _lenght;
  do
  {
    uint32_t sent_n = 0;

    SEGGER_RTT_LOCK();
    {
      unsigned avail = SEGGER_RTT_GetAvailWriteSpace(RTMON_RTT_BUFF_IDX);
      if (avail > sizeof(s_TermSwitch))
      {
        uint32_t send_n = TU_MIN((uint32_t)(avail - sizeof(s_TermSwitch)), (uint32_t)buf_size);
        SEGGER_RTT_WriteNoLock(RTMON_RTT_BUFF_IDX, s_TermSwitch, sizeof(s_TermSwitch));
        sent_n = SEGGER_RTT_WriteNoLock(RTMON_RTT_BUFF_IDX, _buf, send_n);
      }
    }
    SEGGER_RTT_UNLOCK();

    if (sent_n == 0)
    {
      // Channel 0 is full. Back off and retry:
      osal_task_delay(100);
      continue;
    }

    // Iteration:
    _buf += sent_n;
    buf_size -= sent_n;
  } while(buf_size);
}

void rtmon_OnXmitCmplt(void)
{
  osal_semaphore_post(sem_cdc, false);
}

#endif /* defined(RTMON_TUD_CDC_IF) */
