#ifndef __FTG_TRAFFIC_H__
#define __FTG_TRAFFIC_H__

#include <time.h>

struct packet_data
{
	struct timespec delay; /* delay relative to the previous packet */
	size_t size; /* UDP payload size, including fast-tg header */
};

#endif /* __FTG_TRAFFIC_H__ */
