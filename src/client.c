#include <config.h>

#include <arpa/inet.h>
#include <errno.h>
#include <netinet/in.h>
#include <netdb.h>
#include <semaphore.h>
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

/* Data passed to a generator thread */
struct generator_args
{
	/* The generator must place a pointer to the first block here
	 * as soon as possible. */
	struct packet_block **block;
	/* Semaphore for communcation between generator and sending
	 * thread */
	sem_t *sem;
	// Same stuff as passed to run_client, to be replaced by arg
	int size;
	struct timespec *interval;
	/* Custom data (TODO) */
	// void *arg;
};

/* Example of a generator thread */
void* generate_static_block(void *arg);



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

	struct packet_block *block = NULL;
	sem_t *semaphore = malloc(sizeof(sem_t));
	CHKALLOC(semaphore);
	sem_init(semaphore, 0, 0); /* TODO: Error handling */
	pthread_t generator;
	struct generator_args args;
	args.block = &block;
	args.sem = semaphore;
	args.size = size;
	args.interval = interval;
	/* TODO: Error handling */
	pthread_create(&generator, NULL, &generate_static_block, &args);

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

	sem_wait(semaphore);
	pthread_mutex_lock(block->lock);

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
			pthread_mutex_unlock(block->lock);
			block = block->next;
			if (pthread_mutex_trylock(block->lock) != 0)
			{
				fprintf(stderr, "ERROR: Could not get lock for "
					"the next send parameter block!\nMake "
					"sure your data generator is fast "
					"enough and unlocks mutexes "
					"properly.\n");
				break;
			}
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

	pthread_mutex_unlock(block->lock);

	close(sock);
	free(buf);
	pthread_join(generator, NULL);
	free(semaphore);
	packet_block_destroy(block);
	free(block);
}



void* generate_static_block(void *arg)
{
	struct generator_args *args = (struct generator_args *) arg;
	int size = args->size;

	/* TODO: dynamic block length */
#define BLOCK_LEN 10
	struct packet_block *block = malloc(sizeof(struct packet_block));
	CHKALLOC(block);
	packet_block_init(block, BLOCK_LEN);
	block->next = block;

	for (int i = 0; i < block->length; i++)
	{
		block->data[i].size = size;
		memcpy(&(block->data[i].delay), args->interval,
		       sizeof(struct timespec));
	}
	*(args->block) = block;
	sem_post(args->sem);
	return NULL;
}
