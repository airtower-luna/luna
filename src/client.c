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
#include "generator.h"
#include "traffic.h"
#include "simple_generator.h"



/* List of known generators */
#define KNOWN_GENERATORS_LENGTH 3
static struct generator_type known_generators[] = {
	{"static", &static_generator_create},
	{"random_size", &rand_size_generator_create},
	{"alt_time", &alternate_time_generator_create},
};



int run_client(struct addrinfo *addr, int time,
	       char *generator_type, char *generator_args)
{
	printf("Generator: %s\n", generator_type);

	struct packet_block *block = NULL;
	sem_t semaphore;
	sem_t ready_sem;
	sem_init(&semaphore, 0, 0); /* TODO: Error handling */
	sem_init(&ready_sem, 0, 0); /* TODO: Error handling */
	generator_t generator;
	memset(&generator, 0, sizeof(generator_t));
	generator.block = &block;
	generator.control = &semaphore;
	generator.ready = &ready_sem;

	/* initialize the requested generator */
	for (int i = 0; i < KNOWN_GENERATORS_LENGTH; i++)
	{
		if (strcmp(generator_type, known_generators[i].name) == 0)
			known_generators[i].create(&generator, generator_args);
	}
	/* fail if the requested generator is unknown */
	if (generator.init_generator == NULL)
	{
		fprintf(stderr, "ERROR: Unknown generator "
			"\"%s\"!\n", generator_type);
		exit(EXIT_INVALID);
	}

	pthread_t gen_thread;
	/* TODO: Error handling */
	pthread_create(&gen_thread, NULL, &run_generator, &generator);

	/* Allocate buffer, based on upper size limit provided by the
	 * generator */
	char *buf = malloc(generator.max_size);
	CHKALLOC(buf);
	memset(buf, 7, generator.max_size);
	/* TODO: Verify that max_size is large enough for the protocol
	 * header */

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

	sem_wait(&ready_sem);
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
			sem_post(&semaphore);
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
	pthread_cancel(gen_thread);

	close(sock);
	free(buf);

	pthread_join(gen_thread, NULL);
	generator.destroy_generator(&generator);
	sem_destroy(&semaphore);
	sem_destroy(&ready_sem);
}
