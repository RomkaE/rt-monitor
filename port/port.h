
#ifndef RTMON_PORT_H_
#define RTMON_PORT_H_

#include <stddef.h>

#ifndef ESP_PLATFORM
  #include "FreeRTOS.h"
  #include "FreeRTOSConfig.h"
#else
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
#endif

void rtmon_portInit(void);

void rtmon_xmitBuf(const char *_buf, const size_t _lenght);

void rtmon_portInitRunTimer(void);

configRUN_TIME_COUNTER_TYPE rtmon_portGetRunTimer(void);

#endif /* RTMON_PORT_H_ */
