#include <config.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "fast-tg.h"
#include "traffic.h"

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
	       size_t size, int time)
{
	if (size < MIN_PACKET_SIZE)
		size = MIN_PACKET_SIZE;

	/* TODO: allocate buffer based on upper size limit */
	char *buf = malloc(size);
	CHKALLOC(buf);
	memset(buf, 7, size);

	/* TODO: dynamic block length, separate data generation
	 * thread */
#define BLOCK_LEN 10
	struct packet_block *block = malloc(sizeof(struct packet_block));
	CHKALLOC(block);
	packet_block_init(block, BLOCK_LEN);
	block->next = block;
	for (int i = 0; i < block->length; i++)
	{
		block->data[i].size = size;
		memcpy(&(block->data[i].delay), interval,
		       sizeof(struct timespec));
	}

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

	/* current sequence number */
	int seq = 0;
	/* sequence element in the fast-tg packet */
	int *sequence = (int *) buf;
	/* index in the current block */
	int bi = 0;

	/* timespecs for the timer */
	struct timespec nexttick = {0, 0};
	struct timespec rem = {0, 0};
	struct timespec now = {0, 0};
	clock_gettime(CLOCK_MONOTONIC, &nexttick);
	struct timespec end = {nexttick.tv_sec + time, nexttick.tv_nsec};

	/* Store page fault statistics to check if memory management
	 * is working properly */
	struct rusage usage_pre;
	struct rusage usage_post;
	getrusage(RUSAGE_SELF, &usage_pre);
	while (now.tv_sec < end.tv_sec || now.tv_nsec < end.tv_nsec)
	{
		*sequence = htonl(seq++);
		timespecadd(&nexttick, &(block->data[bi].delay), &nexttick);
		clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME,
				&nexttick, &rem); // TODO: error check
		if (send(sock, buf, block->data[bi].size, 0) == -1)
			perror("Error while sending");
		if (++bi == block->length)
		{
			bi = 0;
			block = block->next;
		}
		/* get the current time, needed to stop the loop at
		 * the right time */
		clock_gettime(CLOCK_MONOTONIC, &now);
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

	close(sock);
	free(buf);
	packet_block_destroy(block);
	free(block);
}
