
#include <assert.h>
#include "rtmon_config.h"
#include "../port.h"

// ESP8266 sdk:
#include "sdkconfig.h"
#include "driver/uart.h"

void rtmon_portInit(void)
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
  res = uart_param_config(RTMON_CFG_UART_NUM, &uart_config);
  assert(res == ESP_OK);
  res = uart_driver_install(RTMON_CFG_UART_NUM, 0, 256, 0, NULL, 0);
  assert(res == ESP_OK);
}

void rtmon_xmitBuf(const char *_buf, const size_t _lenght)
{
  int tx_size = uart_write_bytes(RTMON_CFG_UART_NUM, _buf, _lenght);
  assert(tx_size == _lenght);
  esp_err_t res = uart_wait_tx_done(RTMON_CFG_UART_NUM, portMAX_DELAY);
  assert(res == ESP_OK);
}
