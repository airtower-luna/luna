#ifndef __LUNA_CLIENT_H__
#define __LUNA_CLIENT_H__

#include <netinet/in.h>

/*
 * addr: destination (IP address, port)
 * time: time (in seconds) to send packets
 * generator_type: name of the generator to use
 * generator_args: parameters for the generator
 */
int run_client(struct addrinfo *addr, int time, int echo,
	       char *generator_type, char *generator_args,
	       const char *datafile);

#endif /* __LUNA_CLIENT_H__ */
