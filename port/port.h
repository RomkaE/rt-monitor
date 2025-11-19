
#ifndef RTMON_PORT_H_
#define RTMON_PORT_H_

#include <stddef.h>

//#ifndef ESP_PLATFORM
//  #include "FreeRTOS.h"
//#else
//  #include "freertos/FreeRTOS.h"
//#endif

void rtmon_portInit(void);

void rtmon_xmitBuf(const char *_buf, const size_t _size);

void rtmon_OnXmitCmplt(void);

#endif /* RTMON_PORT_H_ */
