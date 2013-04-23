#include <config.h>

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
#include "server.h"

/* size of the buffer for one message */
#define MSG_BUF_SIZE 1500
/* size of the buffer for one sockaddr struct (IPv6 sockaddr is the
 * largest one we should expect) */
#define ADDRBUF_SIZE (sizeof(struct sockaddr_in6))
/* length for address and port strings (probably a bit longer than
 * required) */
#define ADDR_STR_LEN 100
/* buffer size for time string (%T or %s of strftime, with some room
 * to spare for the latter) */
#define T_TIME_BUF 16

int run_server(struct addrinfo *addr, int flags)
{
	/* inet6_only one must be a real variable so it can be used in
	 * setsockopt. */
	int inet6_only = flags & SERVER_IPV6_ONLY;

	/* create the socket */
	int sock;
	struct addrinfo *rp;
	for (rp = addr; rp != NULL; rp = rp->ai_next)
	{
		sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
		if (sock == -1)
			continue; // didn't work, try next address

		if (rp->ai_family == AF_INET6)
			if (setsockopt(sock, IPPROTO_IPV6, IPV6_V6ONLY,
				       &inet6_only, sizeof(inet6_only)) != 0)
			{
				perror("setsockopt IPV6_V6ONLY");
				exit(EXIT_NETFAIL);
			}

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
	CHKALLOC(buf);
	ssize_t recvlen = 0;
	int seq = 0;
	struct sockaddr *addrbuf = malloc(ADDRBUF_SIZE);
	CHKALLOC(addrbuf);
	socklen_t addrlen = 0;
	char *addrstr = malloc(ADDR_STR_LEN);
	CHKALLOC(addrstr);
	char *portstr = malloc(ADDR_STR_LEN);
	CHKALLOC(portstr);

	/* timestamp related data */
	struct timeval ptime;
	struct timeval stime;
	char *tsstr = calloc(T_TIME_BUF, sizeof(char));
	CHKALLOC(tsstr);
	/* *tm will be used to point to localtime's statically
	 * allocated memory, does not need to be allocated/freed
	 * manually */
	struct tm *tm;

	if (flags & SERVER_TSV_OUTPUT)
		printf("# time\tsource\tport\tsequence\tsize\n");

	int work = 1; // could be changed by SIGTERM later
	while (work)
	{
		addrlen = ADDRBUF_SIZE;
		recvlen = recvfrom(sock, buf, buflen, 0, addrbuf, &addrlen);
		ioctl(sock, SIOCGSTAMP, &ptime); // TODO: error check

		if (addrlen > ADDRBUF_SIZE)
			fprintf(stderr, "recv: addr buffer too small!\n");
		/* Create strings for source address and port, disable
		 * name resolution (would require DNS requests). */
		getnameinfo(addrbuf, addrlen,
			    addrstr, ADDR_STR_LEN,
			    portstr, ADDR_STR_LEN,
			    NI_DGRAM | NI_NUMERICHOST | NI_NUMERICSERV); // TODO: error check

		seq = ntohl(*((int *) buf));
		if (flags & SERVER_TSV_OUTPUT)
		{
			tm = localtime(&(ptime.tv_sec));
			strftime(tsstr, T_TIME_BUF, "%s", tm);
			printf("%s.%06ld\t%s\t%s\t%i\t%i\n",
			       tsstr, ptime.tv_usec,
			       addrstr, portstr, seq, (int) recvlen);
		}
		else
		{
			tm = localtime(&(ptime.tv_sec));
			strftime(tsstr, T_TIME_BUF, "%T", tm);
			printf("Received packet %i (%i bytes) from %s, port %s "
			       "at %s.%06ld.\n",
			       seq, (int) recvlen, addrstr, portstr, tsstr,
			       ptime.tv_usec);
		}
	}

	free(tsstr);
	free(tscstr);
	free(addrbuf);
	free(addrstr);
	free(portstr);
	free(buf);
	close(sock);
}
