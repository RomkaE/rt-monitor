
#include <assert.h>
#include "sys_monitor_cfg.h"
#include "../inc/port.h"

// ESP-IDF:
#include "driver/uart.h"
#include "driver/gpio.h"
#ifdef CONFIG_PM_ENABLE
  #include "esp_pm.h"
#endif

#ifdef CONFIG_PM_ENABLE
  static esp_pm_lock_handle_t s_pm_lock;
#endif

void portSysMonitor_Init(void)
{
  esp_err_t res;

  #ifdef CONFIG_PM_ENABLE
    res = esp_pm_lock_create(ESP_PM_NO_LIGHT_SLEEP, (int)0, "smon", &s_pm_lock);
    assert(res == ESP_OK);
  #endif

  const uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
      .source_clk = UART_SCLK_APB
  };

  res = uart_param_config(SYS_MONITOR_UART_NUM, &uart_config);
  assert(res == ESP_OK);
  res = uart_set_pin(SYS_MONITOR_UART_NUM, SYS_MONITOR_TX_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
  assert(res == ESP_OK);
  res = uart_driver_install(SYS_MONITOR_UART_NUM, 256, 256, 0, NULL, 0);
  assert(res == ESP_OK);
}

void portSysMonitor_TxBuff(const void *_buff, uint16_t _lenght)
{
  esp_err_t res;

  #ifdef CONFIG_PM_ENABLE
    res = esp_pm_lock_acquire(s_pm_lock);
    assert(res == ESP_OK);
  #endif

  int tx_size = uart_write_bytes(SYS_MONITOR_UART_NUM, _buff, _lenght);
  assert(tx_size == _lenght);
  res = uart_wait_tx_done(SYS_MONITOR_UART_NUM, portMAX_DELAY);
  assert(res == ESP_OK);

  #ifdef CONFIG_PM_ENABLE
    res = esp_pm_lock_release(s_pm_lock);
    assert(res == ESP_OK);
  #endif
}
