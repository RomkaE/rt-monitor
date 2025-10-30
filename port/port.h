
#ifndef RTMON_PORT_H_
#define RTMON_PORT_H_

#include <stddef.h>
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"

void rtmon_portInit(void);

void rtmon_xmitBuf(const char *_buf, const size_t _lenght);

void rtmon_xmitCmpltCallback(void);

void rtmon_portInitRunTimer(void);

configRUN_TIME_COUNTER_TYPE rtmon_portGetRunTimer(void);

#endif /* RTMON_PORT_H_ */
