#include <config.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

#include "fast-tg.h"
#include "server.h"

/* length for address and port strings (probably a bit longer than
 * required) */
#define ADDR_STR_LEN 100

/* changed to 0 by SIGTERM to stop the receive loop */
int work = 1;

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

	/* configure graceful exit on SIGTERM, if requested */
	if (flags & SERVER_SIGTERM_EXIT)
	{
		struct sigaction act;
		memset(&act, 0, sizeof(struct sigaction));
		act.sa_handler = term_server;
		sigaction(SIGTERM, &act, NULL);
	}

	size_t buflen = MSG_BUF_SIZE;
	char *buf = malloc(buflen);
	CHKALLOC(buf);
	touch_page(buf, buflen);
	ssize_t recvlen = 0;
	int seq = 0;
	struct sockaddr *addrbuf = malloc(ADDRBUF_SIZE);
	CHKALLOC(addrbuf);
	touch_page(addrbuf, ADDRBUF_SIZE);
	socklen_t addrlen = 0;
	char *addrstr = malloc(ADDR_STR_LEN);
	CHKALLOC(addrstr);
	touch_page(addrstr, ADDR_STR_LEN);
	char *portstr = malloc(ADDR_STR_LEN);
	CHKALLOC(portstr);
	touch_page(portstr, ADDR_STR_LEN);

	/* timestamp related data */
	struct timeval ptime;
	char *tsstr = calloc(T_TIME_BUF, sizeof(char));
	CHKALLOC(tsstr);
	touch_page(tsstr, T_TIME_BUF);
#ifdef ENABLE_KUTIME
	struct timeval stime;
	char *tscstr = calloc(T_TIME_BUF, sizeof(char));
	CHKALLOC(tscstr);
	touch_page(tscstr, T_TIME_BUF);
#endif
	/* *tm will be used to point to localtime's statically
	 * allocated memory, does not need to be allocated/freed
	 * manually */
	struct tm *tm;
	/* Call localtime to make sure its internal memory structures
	 * get initialized. The result doesn't matter. */
	localtime(&(ptime.tv_sec));

	/* set up output */
	char *time_trans = calloc(3, sizeof(char));
	CHKALLOC(time_trans);
	if (flags & SERVER_TSV_OUTPUT)
	{
#ifdef ENABLE_KUTIME
		printf("# ktime\tutime\tsource\tport\tsequence\tsize\n");
#else
		printf("# ktime\tsource\tport\tsequence\tsize\n");
#endif
		time_trans = "%s";
	}
	else
		time_trans = "%T";

	/* Store page fault statistics to check if memory management
	 * is working properly */
	struct rusage usage_pre;
	struct rusage usage_post;
	getrusage(RUSAGE_SELF, &usage_pre);

	while (work)
	{
		addrlen = ADDRBUF_SIZE;
		recvlen = recvfrom(sock, buf, buflen, 0, addrbuf, &addrlen);
#ifdef ENABLE_KUTIME
		gettimeofday(&stime, NULL);
#endif
		/* ensure minimum packet size */
		if (recvlen < MIN_PACKET_SIZE)
		{
			fprintf(stderr, "Only %ld bytes received, "
				"smaller than minimum protocol size! "
				"Ignoring packet.\n", recvlen);
			continue;
		}
		/* This only occurs when recvfrom() is cancelled by
		 * SIGTERM. */
		if (recvlen == -1)
			continue;

		/* echo packet if echo flag is set */
		if (buf[sizeof(int) + sizeof(struct timespec)]
		    & FTG_FLAG_ECHO)
			sendto(sock, buf, recvlen, 0, addrbuf, addrlen);

		/* get kernel timestamp */
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

		tm = localtime(&(ptime.tv_sec));
		strftime(tsstr, T_TIME_BUF, time_trans, tm);
#ifdef ENABLE_KUTIME
		tm = localtime(&(stime.tv_sec));
		strftime(tscstr, T_TIME_BUF, time_trans, tm);
#endif

		if (flags & SERVER_TSV_OUTPUT)
		{
#ifdef ENABLE_KUTIME
			printf("%s%06ld\t%s%06ld\t%s\t%s\t%i\t%i\n",
			       tsstr, ptime.tv_usec, tscstr, stime.tv_usec,
			       addrstr, portstr, seq, (int) recvlen);
#else
			printf("%s%06ld\t%s\t%s\t%i\t%i\n",
			       tsstr, ptime.tv_usec,
			       addrstr, portstr, seq, (int) recvlen);
#endif
		}
		else
		{
#ifdef ENABLE_KUTIME
			printf("Received packet %i (%i bytes) from %s, port %s "
			       "at %s.%06ld.\n",
			       seq, (int) recvlen, addrstr, portstr, tsstr,
			       ptime.tv_usec, tscstr, stime.tv_usec);
#else
			printf("Received packet %i (%i bytes) from %s, port %s "
			       "at %s.%06ld.\n",
			       seq, (int) recvlen, addrstr, portstr, tsstr,
			       ptime.tv_usec);
#endif
		}
	}

	/* Check page fault statistics to see if memory management is
	 * working properly */
	getrusage(RUSAGE_SELF, &usage_post);
	if (check_pfaults(&usage_pre, &usage_post))
		fprintf(stderr,
			"WARNING: Page faults occurred in real-time section!\n"
			"Pre:  Major-pagefaults: %ld, Minor Pagefaults: %ld\n"
			"Post: Major-pagefaults: %ld, Minor Pagefaults: %ld\n",
			usage_pre.ru_majflt, usage_pre.ru_minflt,
			usage_post.ru_majflt, usage_post.ru_minflt);

	fflush(NULL);
	free(tsstr);
#ifdef ENABLE_KUTIME
	free(tscstr);
#endif
	free(addrbuf);
	free(addrstr);
	free(portstr);
	free(buf);
	close(sock);
}



void term_server(int signum)
{
    work = 0;
}
