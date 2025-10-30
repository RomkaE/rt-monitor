
#include <assert.h>
#include "sys_monitor_cfg.h"
#include "../inc/port.h"

// ESP8266 sdk:
#include "sdkconfig.h"
#include "driver/uart.h"

void portSysMonitor_Init(void)
{
  uart_config_t uart_config = {
      .baud_rate = 115200,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
	    .rx_flow_ctrl_thresh = 0
  };

  esp_err_t res;
  res = uart_param_config(SYS_MONITOR_UART_NUM, &uart_config);
  assert(res == ESP_OK);
  res = uart_driver_install(SYS_MONITOR_UART_NUM, 0, 256, 0, NULL, 0);
  assert(res == ESP_OK);
}

void portSysMonitor_TxBuff(const void *_buff, uint16_t _lenght)
{
  int tx_size = uart_write_bytes(SYS_MONITOR_UART_NUM, _buff, _lenght);
  assert(tx_size == _lenght);
  esp_err_t res = uart_wait_tx_done(SYS_MONITOR_UART_NUM, portMAX_DELAY);
  assert(res == ESP_OK);
}
