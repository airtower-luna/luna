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
#ifndef __LUNA_LUNA_H__
#define __LUNA_LUNA_H__

#include <stdlib.h>
#include <sys/resource.h>

/* default server port (can be changed by -p command line argument),
 * and it's length (ASCII bytes including terminating null byte) */
#define DEFAULT_PORT 4567
#define DEFAULT_PORT_LEN 6 /* enough for all valid port numbers */

/* the default generator, used if none specified */
#define DEFAULT_GENERATOR "static"

/* exit code for invalid command line arguments */
#define EXIT_INVALID 1
/* exit code for network problems */
#define EXIT_NETFAIL 2
/* exit code for memory errors */
#define EXIT_MEMFAIL 3
/* exit code for file access errors */
#define EXIT_FILEFAIL 4

/* 1s = 1000000µs */
#define US_PER_S 1000000
/* 1µs = 1000ns */
#define NS_PER_US 1000

/*
 * Minimum packet size as required for our payload. Larger sizes are
 * possible as long as the UDP stacks permits them. Current content of
 * the LUNA protocol header:
 *
 * int: sequence number
 * struct timespec: clock time recorded right before sending
 * char: flags byte
 *
 * The two timespec struct components are defined to always be 8 byte
 * each, so this is platform independent.
 */
#define MIN_PACKET_SIZE (sizeof(int) + sizeof(struct timespec) + sizeof(char))
/* set in flags byte to request a response from the server */
#define LUNA_FLAG_ECHO 1

/* size of the buffer for one message */
#define MSG_BUF_SIZE 1500
/* size of the buffer for one sockaddr struct (IPv6 sockaddr is the
 * largest one we should expect) */
#define ADDRBUF_SIZE (sizeof(struct sockaddr_in6))

/* macro and function to check if memory allocation was successful */
#define CHKALLOC(a) chkalloc(a, __FILE__, __LINE__)
void chkalloc(void *ptr, char *file, int line);

/* based on timeradd in <sys/time.h> */
#define timespecadd(a, b, result)					\
	do								\
	{								\
		(result)->tv_sec = (a)->tv_sec + (b)->tv_sec;		\
		(result)->tv_nsec = (a)->tv_nsec + (b)->tv_nsec;	\
		if ((result)->tv_nsec >= 1000000000)			\
		{							\
			++(result)->tv_sec;				\
			(result)->tv_nsec -= 1000000000;		\
		}							\
	} while (0)

/* buffer size for time strings (%T or %s of strftime, with some room
 * to spare for the latter) */
#define T_TIME_BUF 16

/* Compare two struct rusage data sets to see if any page faults
 * occurred in between */
int check_pfaults(struct rusage *pre, struct rusage *post);

/* Touch each page that may be part of mem. The caller is responsible
 * for passing the correct size. */
int touch_page(void *mem, size_t size);

#endif /* __LUNA_LUNA_H__ */
