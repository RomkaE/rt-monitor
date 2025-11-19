
#include <stdbool.h>
#include "rtmon_config.h"
#include "../port.h"
#include "app-error/app_assert.h"
#include "tusb.h"

#if defined(RTMON_TUD_CDC_IF)

static osal_semaphore_def_t sem_def;
static osal_semaphore_t sem_cdc;

void rtmon_portInit(void)
{
  sem_cdc = osal_semaphore_create(&sem_def);
  ASSERT(sem_cdc != NULL);
}

void rtmon_xmitBuf(const char *_buf, const size_t _lenght)
{
//  if (tud_cdc_n_connected(RTMON_TUD_CDC_IF) == false)
//    return;

  size_t buf_size = _lenght;
  do
  {
    uint16_t cdc_size = (uint16_t)tud_cdc_n_write_available(RTMON_TUD_CDC_IF);
    if (cdc_size == 0)
      return;

    // Write data to CDC:
    uint32_t send_n = TU_MIN(cdc_size, buf_size);
    uint32_t sent_n = tud_cdc_n_write(RTMON_TUD_CDC_IF, _buf, send_n);
    ASSERT(send_n == sent_n);
    tud_cdc_n_write_flush(RTMON_TUD_CDC_IF);

    // Wait:
    if (!osal_semaphore_wait(sem_cdc, 1000))
    {
      // TODO - add logs?!
    }

    // Iteration:
    _buf += send_n;
    buf_size -= send_n;
  } while(buf_size);
}

void rtmon_OnXmitCmplt(void)
{
  osal_semaphore_post(sem_cdc, false);
}

#endif /* defined(RTMON_TUD_CDC_IF) */
