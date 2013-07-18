#ifndef __FTG_CLIENT_H__
#define __FTG_CLIENT_H__

#include <netinet/in.h>

/*
 * addr: destination (IP address, port)
 * time: time (in seconds) to send packets
 * generator_type: name of the generator to use
 * generator_args: parameters for the generator
 */
int run_client(struct addrinfo *addr, int time,
	       char *generator_type, char *generator_args);

#endif /* __FTG_CLIENT_H__ */
