#include <config.h>

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
#include <time.h>
#include <unistd.h>

#include "fast-tg.h"
#include "server.h"
#include "client.h"

/* valid command line options for getopt */
#define CLI_OPTS "sc:p:46TS:"

void chkalloc(void *ptr, char *file, int line)
{
	if (ptr == NULL)
	{
		fprintf(stderr,
			"Could not allocate required memory in %s, line %i!\n",
			file, line);
		exit(EXIT_MEMFAIL);
	}
}



void printtimeres()
{
	struct timespec timeres = {0, 0};
	if (clock_getres(CLOCK_MONOTONIC, &timeres) == -1)
		perror("Could not get clock resolution");
	else
		printf("Kernel clock resolution: %ld.%09lds\n",
		       timeres.tv_sec, timeres.tv_nsec);
}



int main(int argc, char *argv[])
{
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

	int server = 0;
	int client = 0;
	int flags = 0;
	int psize = 4;
	/* port and host will be allocated by strdup, free'd below. */
	char *port = NULL;
	char *host = NULL;
	for (int opt = getopt(argc, argv, CLI_OPTS);
	     opt != -1;
	     opt = getopt(argc, argv, CLI_OPTS))
	{
		switch (opt)
		{
		case 'p':
			port = strdup(optarg);
			CHKALLOC(port);
			break;
		case 's': // act as server
			if (client != 0)
			{
				fprintf(stderr, "Select client or server mode, "
					"never both!\n");
				exit(EXIT_INVALID);
			}
			server = 1;
			break;
		case 'c': // act as client
			if (server != 0)
			{
				fprintf(stderr, "Select client or server mode, "
					"never both!\n");
				exit(EXIT_INVALID);
			}
			host = strdup(optarg);
			CHKALLOC(host);
			client = 1;
			break;
		case '4': // IPv4 only
			addrhints.ai_family = AF_INET;
			break;
		case '6': // IPv6 only
			addrhints.ai_family = AF_INET6;
			flags |= SERVER_IPV6_ONLY;
			break;
		case 'T': // tab separated value output
			flags |= SERVER_TSV_OUTPUT;
			break;
		case 'S': // configure packet size (client only)
			psize = atoi(optarg);
			break;
		default:
			break;
		}
	}

	if (!(flags & SERVER_TSV_OUTPUT))
		printtimeres();

	if (port == NULL)
	{
		port = malloc(DEFAULT_PORT_LEN);
		CHKALLOC(port);
		snprintf(port, DEFAULT_PORT_LEN, "%u", DEFAULT_PORT);
	}

	if (server)
	{
		addrhints.ai_flags |= AI_PASSIVE;
		if (addrhints.ai_family != AF_INET)
			addrhints.ai_family = AF_INET6;
	}
	else
		if (host == NULL)
		{
			fprintf(stderr, "You must either use server mode or "
				"specify a server to send to (-c HOST)!\n");
			exit(EXIT_INVALID);
		}

	/* res will be allocated by getaddrinfo and free'd in
	 * client/server functions. */
	struct addrinfo *res = NULL;
	int error = 0;
	error = getaddrinfo(host, port, &addrhints, &res);
	if (error != 0)
	{
		fprintf(stderr, "Error in getaddrinfo for \"%s\": %s\n",
			host, gai_strerror(error));
	}

	free(host);
	free(port);

	if (client)
	{
		struct timespec interval = {0, 1000000};
		return run_client(res, &interval, psize, 1000);
	}

	if (server)
		return run_server(res, flags);
}
