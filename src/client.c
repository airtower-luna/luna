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
#include <linux/sockios.h>
#include <netinet/in.h>
#include <netdb.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "luna.h"
#include "generator.h"
#include "traffic.h"
#include "simple_generator.h"
#include "gaussian_generator.h"



/* List of known generators */
#define KNOWN_GENERATORS_LENGTH 4
static const struct generator_type known_generators[] = {
	{"static", &static_generator_create},
	{"random_size", &rand_size_generator_create},
	{"alt_time", &alternate_time_generator_create},
	{"gaussian", &gaussian_generator_create},
};



#define ECHO_PRIO_OFFSET 2

void* echo_thread(void *arg);
void _fclose_wrapper(void *arg);

struct echo_thread_data
{
	/* socket to read from */
	int sock;
	/* post to this semaphore when init is done */
	sem_t sem;
	/* output file or NULL */
	const char *datafile;
};



int run_client(struct addrinfo *addr, const int time,
	       const struct timespec start_time, const clockid_t clk_id,
	       const int echo,
	       const char *const generator_type,
	       const char *const generator_args,
	       const char *const datafile)
{
	fprintf(stderr, "Generator: %s\n", generator_type);

	sem_t semaphore;
	sem_t ready_sem;
	sem_init(&semaphore, 0, 0); /* TODO: Error handling */
	sem_init(&ready_sem, 0, 0); /* TODO: Error handling */
	generator_t generator;
	memset(&generator, 0, sizeof(generator_t));
	generator.control = &semaphore;
	generator.ready = &ready_sem;

	/* initialize the requested generator */
	for (int i = 0; i < KNOWN_GENERATORS_LENGTH; i++)
	{
		generator_option *const gen_args =
			split_generator_args(generator_args);
		if (strcmp(generator_type, known_generators[i].name) == 0)
			known_generators[i].create(&generator, gen_args);
		free_generator_args(gen_args);
	}
	/* fail if the requested generator is unknown */
	if (generator.init_generator == NULL)
	{
		fprintf(stderr, "ERROR: Unknown generator "
			"\"%s\"!\n", generator_type);
		exit(EXIT_INVALID);
	}

	/* also used for echo thread, if any */
	pthread_attr_t thread_attrs;
	pthread_attr_init(&thread_attrs);

	pthread_attr_setstacksize(&thread_attrs, 2 * PTHREAD_STACK_MIN);
	pthread_t gen_thread;
	int ret = pthread_create(&gen_thread, &thread_attrs, &run_generator, &generator);
	if (ret != 0)
	{
		fprintf(stderr, "creating generator thread failed: %s\n",
			strerror(ret));
		exit(1);
	}

	/* Allocate buffer, based on upper size limit provided by the
	 * generator */
	char *const buf = malloc(generator.max_size);
	CHKALLOC(buf);
	memset(buf, 7, generator.max_size);

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

	/* prepare and start echo handler thread, if requested */
	pthread_t e_thread;
	struct echo_thread_data *e_data = NULL;
	if (echo)
	{
		e_data = malloc(sizeof(struct echo_thread_data));
		CHKALLOC(e_data);
		touch_page(e_data, sizeof(struct echo_thread_data));
		e_data->sock = sock;
		e_data->datafile = datafile;
		sem_init(&(e_data->sem), 0, 0); /* TODO: Error handling */
		ret = pthread_create(&e_thread, &thread_attrs, &echo_thread, e_data);
		if (ret != 0) {
			fprintf(stderr, "creating echo thread failed: %s\n",
				strerror(ret));
			exit(1);
		}
	}
	pthread_attr_destroy(&thread_attrs);

	/* current sequence number */
	int seq = 0;
	/* sequence number in the LUNA packet */
	int *const sequence = (int *) buf;
	/* time right before sending in the LUNA packet */
	struct timespec *const sendtime =
		(struct timespec *) (buf + sizeof(int));
	/* protocol flags field */
	char *const flags =
		(char *) (buf + sizeof(int) + sizeof(struct timespec));
	*flags = 0;
	if (echo)
		*flags = *flags | LUNA_FLAG_ECHO;
	/* index in the current block */
	int bi = 0;

	sem_wait(&ready_sem);
	if (echo)
		sem_wait(&(e_data->sem));
	struct packet_block *block = generator.block;
	pthread_mutex_lock(block->lock);

	/* timespecs for the timer */
	struct timespec nexttick = {0, 0};
	struct timespec rem = {0, 0};
	struct timespec now = {0, 0};

	/* if start_time is zeroed, just use "now" */
	if (start_time.tv_sec == 0 && start_time.tv_nsec == 0)
		clock_gettime(clk_id, &nexttick);
	/* otherwise initialize nexttick to start_time */
	else
	{
		nexttick.tv_sec = start_time.tv_sec;
		nexttick.tv_nsec = start_time.tv_nsec;
	}
	const struct timespec end = {nexttick.tv_sec + time, nexttick.tv_nsec};

	/* Store page fault statistics to check if memory management
	 * is working properly */
	struct rusage usage_pre;
	struct rusage usage_post;
	getrusage(RUSAGE_SELF, &usage_pre);
	while (now.tv_sec < end.tv_sec || now.tv_nsec < end.tv_nsec)
	{
		*sequence = htonl(seq++);
		timespecadd(&nexttick, &(block->data[bi].delay), &nexttick);
		/* sleep until scheduled send time */
		clock_nanosleep(clk_id, TIMER_ABSTIME,
				&nexttick, &rem); // TODO: error check
		/* record current time into the packet */
		clock_gettime(CLOCK_REALTIME, sendtime);
		/* send the packet */
		if (send(sock, buf, block->data[bi].size, 0) == -1)
			perror("Error while sending");

		/* switch buffer block if necessary */
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
		clock_gettime(clk_id, &now);
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

	/* send buffer isn't needed any more */
	free(buf);

	/* free up generator resources after it has terminated */
	pthread_join(gen_thread, NULL);
	generator.destroy_generator(&generator);
	sem_destroy(&semaphore);
	sem_destroy(&ready_sem);

	if (echo)
	{
		// TODO: appropriate delay to wait for echos
		pthread_cancel(e_thread);
		/* wait for echo handler thread to terminate and free
		 * associated data */
		pthread_join(e_thread, NULL);
		sem_destroy(&(e_data->sem));
		free(e_data);
	}

	/* close socket after echo thread has terminated */
	close(sock);
}



/* This function should run in a separate thread to handle echo
 * packets */
void* echo_thread(void *arg)
{
	struct echo_thread_data *data = (struct echo_thread_data *) arg;
	int sock = data->sock;

	/* Processing echo packets is less urgent than sending or
	 * generation, because the kernel buffers them. Reduce
	 * priority by up to ECHO_PRIO_OFFSET, as long as it remains
	 * within the general priority limits. */
	const pthread_t self = pthread_self();
	int sched_policy = 0;
	struct sched_param sched_param;
	pthread_getschedparam(self, &sched_policy, &sched_param);
	const int min_prio = sched_get_priority_min(sched_policy);
	if (sched_param.sched_priority - ECHO_PRIO_OFFSET < min_prio)
		pthread_setschedprio(self, min_prio);
	else
		pthread_setschedprio
			(self, sched_param.sched_priority - ECHO_PRIO_OFFSET);

	/* allocate receive buffer */
	size_t buflen = MSG_BUF_SIZE;
	char *const buf = malloc(buflen);
	CHKALLOC(buf);
	touch_page(buf, buflen);
	/* ensure free() on cancellation */
	pthread_cleanup_push(&free, buf);
	ssize_t recvlen = 0;
	int seq = 0;
	struct sockaddr *const addrbuf = malloc(ADDRBUF_SIZE);
	CHKALLOC(addrbuf);
	touch_page(addrbuf, ADDRBUF_SIZE);
	pthread_cleanup_push(&free, addrbuf);
	socklen_t addrlen = 0;
	/* timestamp related data */
	struct timespec *const sendtime =
		(struct timespec *) (buf + sizeof(int));
	struct timeval recvtime = {0, 0};
	struct timeval rtt = {0, 0};

	/* prepare time to text conversion */
	tzset();
	struct tm tm;
	localtime_r(&(rtt.tv_sec), &tm);
	char *const timestr = calloc(T_TIME_BUF, sizeof(char));
	CHKALLOC(timestr);
	touch_page(timestr, T_TIME_BUF);
	pthread_cleanup_push(&free, timestr);

	/* Open output file if specified */
	FILE *dataout = stdout;
	if (data->datafile != NULL)
	{
		dataout = fopen(data->datafile, "w");
		if (dataout == NULL)
		{
			perror("Opening output file in echo_thread");
			exit(EXIT_FILEFAIL);
		}
	}
	pthread_cleanup_push(&_fclose_wrapper, dataout);

	int work = 1;
	fprintf(dataout, "# ktime\tsequence\tsize\trtt\n");
	/* init done */
	sem_post(&(data->sem));

	while (work)
	{
		addrlen = ADDRBUF_SIZE;
		/* recvfrom() is a cancellation point according to
		 * POSIX, so handling the -1 return case is not
		 * necessary here. */
		recvlen = recvfrom(sock, buf, buflen, 0, addrbuf, &addrlen);
		/* get kernel timestamp */
		ioctl(sock, SIOCGSTAMP, &recvtime); // TODO: error check

		if (addrlen > ADDRBUF_SIZE)
			fprintf(stderr, "recv: addr buffer too small!\n");

		/* This really should not happen, but we're dealing
		 * with an open network socket, so who knows what
		 * might arrive there... */
		if (recvlen < MIN_PACKET_SIZE)
		{
			fprintf(stderr, "Only %ld bytes received, smaller than "
				"minimum protocol size! Ignoring packet.\n",
				recvlen);
			continue;
		}

		seq = ntohl(*((int *) buf));

		/* Calculate RTT */
		rtt.tv_sec = recvtime.tv_sec - sendtime->tv_sec;
		rtt.tv_usec =
			recvtime.tv_usec - (sendtime->tv_nsec / NS_PER_US);
		if (rtt.tv_usec < 0)
		{
			rtt.tv_usec += US_PER_S;
			rtt.tv_sec -= 1;
		}

		/* Process arrival time */
		localtime_r(&(recvtime.tv_sec), &tm);
		strftime(timestr, T_TIME_BUF, "%s", &tm);

		/* Write packet information */
		fprintf(dataout, "%s%06ld\t%i\t%ld\t",
			timestr, recvtime.tv_usec, seq, recvlen);
		if (rtt.tv_sec > 0)
			fprintf(dataout, "%ld%06ld\n", rtt.tv_sec, rtt.tv_usec);
		else
			fprintf(dataout, "%ld\n", rtt.tv_usec);
	}

	/* The function should never reach this point because it will
	 * be cancelled rather than stopping the loop, but each
	 * pthread_cleanup_push() requires a corresponding
	 * pthread_cleanup_pop(). Cleanup handlers will, of course, be
	 * called upon cancellation. */
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
	pthread_cleanup_pop(1);
}



/* This wrapper is needed because fclose does not match the signature
 * required for pthread_cleanup_push(). If an error occurs, an error
 * message is written, but no further action taken. Since this
 * function is only called when the process is about to exit, this
 * should be safe. */
void _fclose_wrapper(void *arg)
{
	if (arg == stdout)
		return;

	if (fclose((FILE *) arg))
	{
		perror("Closing output file on echo_thread termination");
		exit(EXIT_FILEFAIL);
	}
}
