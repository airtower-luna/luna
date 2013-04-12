#ifndef __FTG_CLIENT_H__
#define __FTG_CLIENT_H__

#include <netinet/in.h>

int run_client(struct sockaddr_in6 *addr, int interval, size_t size, int count);

#endif /* __FTG_CLIENT_H__ */
