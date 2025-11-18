
#include <stdbool.h>
#include "rtmon_config.h"
#include "../port.h"
#include "app-error/app_assert.h"
#include "tusb.h"

// TODO - move to STM32 port
#include "stm32u0xx_hal.h"

#if defined(RTMON_TUD_CDC_IF)

void rtmon_portInit(void)
{

}

void rtmon_xmitBuf(const char *_buf, const size_t _lenght)
{

}

// TODO - move to STM32 port
void rtmon_portInitRunTimer(void)
{
  extern void MX_TIM2_Init(void);
  MX_TIM2_Init();
}

// TODO - move to STM32 port
configRUN_TIME_COUNTER_TYPE rtmon_portGetRunTimer(void)
{
  extern TIM_HandleTypeDef htim2;
  configRUN_TIME_COUNTER_TYPE cnt = __HAL_TIM_GET_COUNTER(&htim2);
  return cnt;
}

bool sysmon_xmit_poll(tu_fifo_t *_fifo)
{
  if (tud_cdc_n_connected(RTMON_TUD_CDC_IF) == false)
    return 0;
  uint16_t ff_size = tu_fifo_count(_fifo);
  if (ff_size == 0)
    return false;

  uint16_t cdc_size = (uint16_t)tud_cdc_n_write_available(RTMON_TUD_CDC_IF);
  if (cdc_size == 0)
    return true;

  uint16_t b_size = TU_MIN(cdc_size, ff_size);
  uint8_t buff[b_size];

  // Read data from FIFO:
  uint16_t ff_read_n = tu_fifo_read_n(_fifo, buff, b_size);
  ASSERT(ff_read_n == b_size);

  // Write data to CDC:
  uint16_t ff_write_n = tud_cdc_n_write(RTMON_TUD_CDC_IF, buff, ff_read_n);
  ASSERT(ff_write_n == ff_read_n);

  tud_cdc_n_write_flush(RTMON_TUD_CDC_IF);

  return true;
}

#endif /* CFG_TUD_ENABLED && CFG_TUD_CDC */
