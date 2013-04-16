#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include "server.h"
#include "client.h"

/* default server port (can be changed by -p command line argument) */
#define DEFAULT_PORT 4567

/* exit code for invalid command line arguments */
#define EXIT_INVALID 1



/*
 * Resolve the hostname to an IPv6 address, write the resulting
 * address to the provided the provided structure.
 */
int resolve_ipv6(char *host, struct sockaddr_in6 *addr)
{
	struct addrinfo hints;
	hints.ai_family = AF_INET6;
	hints.ai_socktype = 0;
	hints.ai_protocol = 0;
	hints.ai_flags = AI_V4MAPPED;
	hints.ai_addrlen = 0;
	hints.ai_addr = NULL;
	hints.ai_canonname = NULL;
	hints.ai_next = NULL;

	struct addrinfo *res = NULL;
	int error = 0;
	error = getaddrinfo(host, "2345", &hints, &res);
        if (error != 0)
        {
		fprintf(stderr, "Error in getaddrinfo for \"%s\": %s\n", host,
			gai_strerror(error));
        }
	// TODO: check that res->ai_family is AF_INET6
	printf("test\n");
	if (res != NULL && res->ai_addrlen == sizeof(struct sockaddr_in6))
	{
		printf("test2\n");
		addr->sin6_addr = ((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
		printf("test3\n");
	}

	printf("end\n");
	freeaddrinfo(res);
	printf("end2\n");
	return 0;
}



int main(int argc, char *argv[])
{
	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(DEFAULT_PORT);
	addr.sin6_addr = in6addr_loopback;

	int opt = getopt(argc, argv, "sc:p:");
	int server = 0;
	int client = 0;
	while (opt != -1)
	{
		switch (opt)
		{
		case 'p':
			addr.sin6_port = htons(atoi(optarg)); // TODO: error check
			break;
		case 's': // act as server
			if (client != 0)
			{
				fprintf(stderr, "Select client or server mode, never both!\n");
				exit(EXIT_INVALID);
			}
			server = 1;
			addr.sin6_addr = in6addr_any;
			break;
		case 'c': // act as client
			if (server != 0)
			{
				fprintf(stderr, "Select client or server mode, never both!\n");
				exit(EXIT_INVALID);
			}
			resolve_ipv6(optarg, &addr);
			client = 1;
			break;
		default:
			break;
		}
		opt = getopt(argc, argv, "scp:");
	}

	if (client)
		return run_client(&addr, 1000, 4, 20);
	if (server)
		return run_server(&addr);
}
