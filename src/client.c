#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>



/*
 * addr: destination (IP address, port)
 * interval: time between two packets (µs)
 * size: packet size in bytes (must be at least 4)
 * count: number of packets to send
 */
int run_client(struct addrinfo *addr, int interval, size_t size, int count)
{
	/* Ensure that size is at least 4, otherwise segfault or
	 * memory corruption might occur. */
	if (size < 4)
		size = 4;
	char *buf = malloc(size); // TODO: error check

	struct addrinfo *rp;
	int sock;
	for (rp = addr; rp != NULL; rp = rp->ai_next)
	{
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sock == -1)
			continue; // didn't work, try next address

		if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1)
			break; // connected (well, it's UDP, but...
	}

	int *seq = (int *) buf;
	/* sleep times */
	struct timespec req = {interval / 1000000, interval * 1000};
	struct timespec rem = {0, 0};
	for (int i = 0; i < count; i++)
	{
		*seq = htonl(i);
		if (send(sock, buf, size, 0) == -1)
			perror("Error while sending");
		nanosleep(&req, &rem); // TODO: error check
	}

	close(sock);
	free(buf);
}
