#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "fast-tg.h"

/* size of the buffer for one message */
#define MSG_BUF_SIZE 1500
/* length for address string */
#define ADDR_STR_LEN 100
/* buffer size for time string (%T of strftime) */
#define T_TIME_BUF 10

int run_server(struct addrinfo *addr)
{
	int sock;
	struct addrinfo *rp;
	for (rp = addr; rp != NULL; rp = rp->ai_next)
	{
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sock == -1)
			continue; // didn't work, try next address

		if (bind(sock, rp->ai_addr, rp->ai_addrlen) == 0)
			break; // connected (well, it's UDP, but...)

		close(sock);
	}
	if (rp == NULL)
	{
		fprintf(stderr, "Could not bind listening socket.\n");
		exit(EXIT_NETFAIL);
	}
	freeaddrinfo(addr); // no longer required

	size_t buflen = MSG_BUF_SIZE;
	void *buf = malloc(buflen);
	ssize_t recvlen = 0;
	int seq = 0;
	int addrbuf_size = sizeof(struct sockaddr_in6);
	struct sockaddr_in6 *addrbuf = malloc(addrbuf_size);
	socklen_t addrlen = 0;
	char *addrstr = malloc(ADDR_STR_LEN);
	uint16_t sourceport = 0;

	/* timestamp related data */
	struct timeval ptime;
	struct timeval stime;
	char *tsstr = calloc(T_TIME_BUF, sizeof(char));
	char *tscstr = calloc(T_TIME_BUF, sizeof(char));
	/* *tm will be used to point to localtime's statically
	 * allocated memory, does not need to be allocated/freed
	 * manually */
	struct tm *tm;

	int work = 1; // could be changed by SIGTERM later
	while (work)
	{
		addrlen = addrbuf_size;
		recvlen = recvfrom(sock, buf, buflen, 0,
				   (struct sockaddr *) addrbuf, &addrlen);
		gettimeofday(&stime, NULL);
		ioctl(sock, SIOCGSTAMP, &ptime); // TODO: error check

		if (addrlen > addrbuf_size)
			fprintf(stderr, "recv: addr buffer too small!\n");
		inet_ntop(AF_INET6, &(addrbuf->sin6_addr), addrstr, ADDR_STR_LEN); // TODO: error check
		sourceport = ntohs(addrbuf->sin6_port);

		seq = ntohl(*((int *) buf));
		tm = localtime(&(ptime.tv_sec));
		strftime(tsstr, T_TIME_BUF, "%T", tm); // TODO: error check
		tm = localtime(&(stime.tv_sec));
		strftime(tscstr, T_TIME_BUF, "%T", tm); // TODO: error check
		printf("Received packet %i (%i bytes) from %s, port %i at %s.%06ld (kernel), %s.%06ld (user space).\n",
		       seq, (int) recvlen, addrstr, sourceport, tsstr, ptime.tv_usec, tscstr, stime.tv_usec);
	}

	free(tsstr);
	free(tscstr);
	free(addrbuf);
	free(addrstr);
	free(buf);
	close(sock);
}
