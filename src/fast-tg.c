#include <config.h>

#include <arpa/inet.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sched.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "fast-tg.h"
#include "server.h"
#include "client.h"

/* valid command line options for getopt */
#define CLI_OPTS "sc:p:46TS:i:t:"

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



int check_pfaults(struct rusage *pre, struct rusage *post)
{
	if (pre->ru_majflt != post->ru_majflt
	    || pre->ru_minflt != post->ru_minflt)
		return 1;
	else
		return 0;
}



int touch_page(void *mem, size_t size)
{
	/* get page size (once) */
	static long page_size = 0;
	if (page_size == 0)
		page_size = sysconf(_SC_PAGESIZE);

	/* touch each page in mem */
	char *c = mem;
	for (int i = 0; i < size; i += page_size)
		c[i] = 0;
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
	int flags = SERVER_SIGTERM_EXIT;
	int psize = 4;
	int interval = 1000;
	int time = 1;
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
		case 'i': // configure packet interval (µs, client only)
			interval = atoi(optarg);
			break;
		case 't': // time to run (s, client only)
			time = atoi(optarg);
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

	int retval = 0;
	/* lock all allocated memory into RAM before starting realtime
	 * sections (needs capability CAP_IPC_LOCK) */
	if (mlockall(MCL_CURRENT | MCL_FUTURE))
	{
		perror("mlockall failed");
		fprintf(stderr, "Make sure fast-tg has the CAP_IPC_LOCK "
			"capability! If you don't need real-time "
			"precision, you can safely ignore this warning.\n");
	}

	/* Try to get real-time priority */
	struct sched_param sparam;
	memset(&sparam, 0, sizeof(struct sched_param));
	/* POSIX requires a minimum range of 32 for priorities. Using
	 * the minimum plus 20 seems reasonable to aquire a high
	 * priority without blocking any high priority processes that
	 * might be running. */
	sparam.sched_priority = sched_get_priority_min(SCHED_RR) + 20;
	if (sched_setscheduler(0, SCHED_RR, &sparam) == -1)
	{
		perror("Could not get real-time priority");
		fprintf(stderr, "Make sure fast-tg has the CAP_SYS_NICE "
			"capability! If you don't need real-time "
			"precision, you can safely ignore this warning.\n");
	}

	if (client)
	{
		struct timespec in;
		in.tv_sec = interval / US_PER_S;
		in.tv_nsec = (interval % US_PER_S) * 1000;
		retval = run_client(res, &in, psize, time);
	}

	if (server)
		retval = run_server(res, flags);

	return retval;
}
