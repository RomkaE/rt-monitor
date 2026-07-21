
#ifndef RTMON_PORT_H_
#define RTMON_PORT_H_

#include <stddef.h>

void rtmon_portInit(void);

void rtmon_xmitBuf(const char *_buf, const size_t _size);

#endif /* RTMON_PORT_H_ */
