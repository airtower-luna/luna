#ifndef __FTG_SERVER_H_
#define __FTG_SERVER_H_

#include <netinet/in.h>

/* option flags for the server */
#define SERVER_IPV6_ONLY 1
#define SERVER_TSV_OUTPUT 2
#define SERVER_SIGTERM_EXIT 4

int run_server(struct addrinfo *addr, int flags);

void term_server(int signum);

#endif /* __FTG_SERVER_H_ */
