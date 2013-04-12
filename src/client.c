#include <sys/socket.h>
#include <netinet/in.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>



/*
 * addr: destination (IP address, port)
 * interval: time between two packets (µs)
 * size: packet size in bytes (must be at least 4)
 * count: number of packets to send
 */
int run_client(struct sockaddr_in6 *addr, int interval, size_t size, int count)
{
	/* Ensure that size is at least 4, otherwise segfault or
	 * memory corruption might occur. */
	if (size < 4)
		size = 4;
	char *buf = malloc(size); // TODO: error check

	int sock = socket(AF_INET6, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == -1)
		perror("Error while creating socket");

	if (connect(sock, (struct sockaddr *) addr, sizeof(*addr)) == -1)
		perror("Error setting destination");

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
