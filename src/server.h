#ifndef __FTG_SERVER_H_
#define __FTG_SERVER_H_

#include <netinet/in.h>

int run_server(struct addrinfo *addr, int inet6_only);

#endif /* __FTG_SERVER_H_ */
