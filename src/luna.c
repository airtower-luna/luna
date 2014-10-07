/*
 * This file is part of the Lightweight Universal Network Analyzer (LUNA)
 *
 * Copyright (c) 2013 Fiona Klute
 *
 * LUNA is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LUNA is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
 * License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LUNA. If not, see <http://www.gnu.org/licenses/>.
 */
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

#include "luna.h"
#include "server.h"
#include "client.h"

/* POSIX requires a minimum range of 32 for priorities. Using the
 * minimum plus 20 seems reasonable to aquire a high priority without
 * blocking any high priority processes that might be running. The
 * client needs a higher priority than the server because the server's
 * time measurements rely on the kernel and thus it has no reason to
 * block the client. A client offset of 60 ensures that it can preempt
 * medium priority kernel threads under PREEMPT_RT, which run at a
 * priority of 50. */
#define CLIENT_PRIO_OFFSET 60
#define SERVER_PRIO_OFFSET 20

/* Option codes for long options, starting at 260 to be safely above
 * charcodes for short options */
#define OPT_START_TIME 260
#define OPT_CLOCK 261

/* valid command line options for getopt */
#define CLI_OPTS "sc:p:46Tt:g:a:eo:"
const struct option long_opts[] = {
	{"server",	no_argument,		NULL,	's'},
	{"client",	required_argument,	NULL,	'c'},
	{"port",	required_argument,	NULL,	'p'},
	{"ipv4",	no_argument,		NULL,	'4'},
	{"ipv6",	no_argument,		NULL,	'6'},
	{"tsv-output",	no_argument,		NULL,	'T'},
	{"time",	required_argument,	NULL,	't'},
	{"generator",	required_argument,	NULL,	'g'},
	{"generator-args", required_argument,	NULL,	'a'},
	{"echo",	no_argument,		NULL,	'e'},
	{"output",	required_argument,	NULL,	'o'},
	{"start-time",	required_argument,	NULL,	OPT_START_TIME},
	{"clock",	required_argument,	NULL,	OPT_CLOCK},
	{NULL,		0,			NULL,	0}
};

void chkalloc(void *const ptr, const char *const file, const int line)
{
	if (ptr == NULL)
	{
		fprintf(stderr,
			"Could not allocate required memory in %s, line %i!\n",
			file, line);
		exit(EXIT_MEMFAIL);
	}
}



int check_pfaults(const struct rusage *const pre,
		  const struct rusage *const post)
{
	if (pre->ru_majflt != post->ru_majflt
	    || pre->ru_minflt != post->ru_minflt)
		return 1;
	else
		return 0;
}



int touch_page(void *const mem, const size_t size)
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
		fprintf(stderr, "Kernel clock resolution: %ld.%09lds\n",
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
	int time = 1;
	struct timespec start_time = {0, 0};
	clockid_t clk_id = CLOCK_MONOTONIC;
	int echo = 0;
	/* port, host and clock will be allocated by strdup if needed,
	 * free'd below. */
	char *port = NULL;
	char *host = NULL;
	char *clock = NULL;
	/* the packet generator to use and its arguments */
	char *generator = NULL;
	char *gen_args = NULL;
	/* file path to write output data to, will be overwritten */
	char *datafile = NULL;

	for (int opt = getopt_long(argc, argv, CLI_OPTS, long_opts, NULL);
	     opt != -1;
	     opt = getopt_long(argc, argv, CLI_OPTS, long_opts, NULL))
	{
		switch (opt)
		{
		case 'p':
			ASSERT_UNINIT(port, "-p");
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
			ASSERT_UNINIT(host, "-c");
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
		case 't': // time to run (s, client only)
			time = atoi(optarg);
			break;
		case 'g': // generator to use (client only)
			ASSERT_UNINIT(generator, "-g");
			generator = strdup(optarg);
			CHKALLOC(generator);
			break;
		case 'a': // arguments for the generator (client only)
			ASSERT_UNINIT(gen_args, "-a");
			gen_args = strdup(optarg);
			CHKALLOC(gen_args);
			break;
		case 'e': // request echo (client only)
			echo = 1;
			break;
		case 'o': // data output file
			ASSERT_UNINIT(datafile, "-o");
			datafile = strdup(optarg);
			CHKALLOC(datafile);
			break;
		case OPT_START_TIME:
			start_time.tv_sec = strtoul(optarg, NULL, 10);
			clk_id = CLOCK_REALTIME;
			break;
		case OPT_CLOCK:
			ASSERT_UNINIT(clock, "--clock");
			clock = strdup(optarg);
			CHKALLOC(clock);
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

	if (generator == NULL)
	{
		generator = strdup(DEFAULT_GENERATOR);
		CHKALLOC(generator);
	}

	/* If true, the clock to use was set manually */
	if (clock != NULL)
	{
		if (strcmp(clock, "realtime") == 0)
			clk_id = CLOCK_REALTIME;
		else if (strcmp(clock, "monotonic") == 0)
			clk_id = CLOCK_MONOTONIC;
		else
		{
			fprintf(stderr, "Invalid clock: \"%s\"!\n", clock);
			exit(EXIT_INVALID);
		}
		free(clock);

		/* If one of the elements of start_time is non-zero,
		 * it was specified on the command line. Right now,
		 * only full seconds are possible, but by checking for
		 * nanoseconds, too, the code is prepared for higher
		 * precision. */
		if ((start_time.tv_sec != 0 || start_time.tv_nsec != 0)
		    && clk_id == CLOCK_MONOTONIC)
		{
			fprintf(stderr, "Error: Using a fixed start time is "
				"not possible with the monotonic clock!\n");
			exit(EXIT_INVALID);
		}
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
		fprintf(stderr, "Make sure LUNA has the CAP_IPC_LOCK "
			"capability! If you don't need real-time "
			"precision, you can safely ignore this warning.\n");
	}

	/* Try to get real-time priority */
	struct sched_param sparam;
	memset(&sparam, 0, sizeof(struct sched_param));
	if (client)
		sparam.sched_priority =
			sched_get_priority_min(SCHED_RR) + CLIENT_PRIO_OFFSET;
	else
		sparam.sched_priority =
			sched_get_priority_min(SCHED_RR) + SERVER_PRIO_OFFSET;
	if (sched_setscheduler(0, SCHED_RR, &sparam) == -1)
	{
		perror("Could not get real-time priority");
		fprintf(stderr, "Make sure LUNA has the CAP_SYS_NICE "
			"capability! If you don't need real-time "
			"precision, you can safely ignore this warning.\n");
	}

	if (client)
		retval = run_client(res, time,
				    start_time, clk_id,
				    echo,
				    generator, gen_args,
				    datafile);
	free(gen_args);
	free(generator);

	if (server)
		retval = run_server(res, flags, datafile);
	free(datafile);

	return retval;
}
