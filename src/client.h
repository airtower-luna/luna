#ifndef __FTG_CLIENT_H__
#define __FTG_CLIENT_H__

#include <netinet/in.h>

int run_client(struct addrinfo *addr, struct timespec *interval,
	       size_t size, int time);

#endif /* __FTG_CLIENT_H__ */
