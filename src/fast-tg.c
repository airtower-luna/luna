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

/* default server port (can be changed by -p command line argument),
 * and it's length (ASCII bytes including terminating null byte) */
#define DEFAULT_PORT 4567
#define DEFAULT_PORT_LEN 6 /* enough for all valid port numbers */

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
	if (res != NULL && res->ai_addrlen == sizeof(struct sockaddr_in6))
	{
		addr->sin6_addr = ((struct sockaddr_in6 *) res->ai_addr)->sin6_addr;
	}

	freeaddrinfo(res);
	return 0;
}



int main(int argc, char *argv[])
{
	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(DEFAULT_PORT);
	addr.sin6_addr = in6addr_loopback;

	struct addrinfo addrhints;
	addrhints.ai_family = AF_UNSPEC;
	addrhints.ai_socktype = SOCK_DGRAM;
	addrhints.ai_protocol = IPPROTO_UDP;
	//	addrhints.ai_flags = AI_V4MAPPED | AI_ADDRCONFIG;
	addrhints.ai_flags = AI_V4MAPPED;
	addrhints.ai_addrlen = 0;
	addrhints.ai_addr = NULL;
	addrhints.ai_canonname = NULL;
	addrhints.ai_next = NULL;

	int opt = getopt(argc, argv, "sc:p:");
	int server = 0;
	int client = 0;
	/* port and host will be allocated by strdup, free'd below. */
	char *port = NULL;
	char *host = NULL;
	while (opt != -1)
	{
		switch (opt)
		{
		case 'p':
			addr.sin6_port = htons(atoi(optarg)); // TODO: error check
			port = strdup(optarg);
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
			host = strdup(optarg);
			client = 1;
			break;
		default:
			break;
		}
		opt = getopt(argc, argv, "scp:");
	}

	if (port == NULL)
	{
		port = malloc(DEFAULT_PORT_LEN);
		snprintf(port, DEFAULT_PORT_LEN, "%u", DEFAULT_PORT);
	}

	if (server)
		addrhints.ai_flags |= AI_PASSIVE;
	else
		if (host == NULL)
		{
			fprintf(stderr, "You must either use server mode or specify a server to send to (-c HOST)!\n");
			exit(EXIT_INVALID);
		}

	struct addrinfo *res = NULL;
	int error = 0;
	error = getaddrinfo(host, port, &addrhints, &res);
	if (error != 0)
	{
		fprintf(stderr, "Error in getaddrinfo for \"%s\": %s\n", host,
			gai_strerror(error));
	}

	free(host);
	free(port);

	if (client)
		return run_client(res, 1000, 4, 20);
	if (server)
		return run_server(&addr);

	/* TODO: after client and server use addrinfo, move this to
	 * after their socket init */
	freeaddrinfo(res);
}
