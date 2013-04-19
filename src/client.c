#include <config.h>

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

#include "fast-tg.h"

/* Minimum packet size as required for our payload. Larger sizes are
 * possible as long as the UDP stacks permits them. */
#define MIN_PACKET_SIZE 4



/*
 * addr: destination (IP address, port)
 * interval: time between two packets (µs)
 * size: packet size in bytes (must be at least 4)
 * count: number of packets to send
 */
int run_client(struct addrinfo *addr, struct timespec *interval,
	       size_t size, int count)
{
	if (size < MIN_PACKET_SIZE)
		size = MIN_PACKET_SIZE;
	char *buf = malloc(size);
	CHKALLOC(buf);

	struct addrinfo *rp;
	int sock;
	for (rp = addr; rp != NULL; rp = rp->ai_next)
	{
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sock == -1)
			continue; // didn't work, try next address

		if (connect(sock, rp->ai_addr, rp->ai_addrlen) != -1)
			break; // connected (well, it's UDP, but...)

		close(sock);
	}
	if (rp == NULL)
	{
		fprintf(stderr, "Could not create socket.\n");
		exit(EXIT_NETFAIL);
	}
	freeaddrinfo(addr); // no longer required

	int *seq = (int *) buf;
	/* timespecs for the timer */
	struct timespec nexttick = {0, 0};
	struct timespec rem = {0, 0};
	clock_gettime(CLOCK_MONOTONIC, &nexttick);

	for (int i = 0; i < count; i++)
	{
		*seq = htonl(i);
		timespecadd(&nexttick, interval, &nexttick);
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
				&nexttick, &rem); // TODO: error check
		if (send(sock, buf, size, 0) == -1)
			perror("Error while sending");
	}

	close(sock);
	free(buf);
}
