#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <getopt.h>
#include <arpa/inet.h>

#include "server.h"
#include "client.h"

/* default server port (can be changed by -p command line argument) */
#define DEFAULT_PORT 4567

/* exit code for invalid command line arguments */
#define EXIT_INVALID 1



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
			addr.sin6_port = htons(atoi(optarg));
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
			// TODO: evaluate optarg into server address
			client = 1;
			break;
		default:
			break;
		}
		opt = getopt(argc, argv, "scp:");
	}

	if (client)
		return run_client(&addr);
	if (server)
		return run_server(&addr);
}
