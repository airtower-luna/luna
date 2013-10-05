#include <config.h>

#include <string.h>
#include <stdlib.h>

#include "fast-tg.h"
#include "generator.h"

void* run_generator(void *arg)
{
	struct generator_t *generator = (struct generator_t *) arg;

	/* The generator thread initially inherits its priority from
	 * the sending thread. However, realtime behavior is more
	 * important for sending, while data generation only has to
	 * keep up well enough to prevent buffer underruns. So, if the
	 * RT priority is above the minimum, reduce it by one to make
	 * sure that the generator will never block the sending
	 * thread. */
	pthread_t self = pthread_self();
	int sched_policy = 0;
	struct sched_param sched_param;
	pthread_getschedparam(self, &sched_policy, &sched_param);
	if (sched_param.sched_priority > sched_get_priority_min(sched_policy))
		pthread_setschedprio(self, sched_param.sched_priority - 1);

	generator->init_generator(generator);
	struct packet_block *block = generator->block;

	sem_post(generator->ready);

	/* this loop will be stopped by thread cancellation */
	while (1)
	{
		sem_wait(generator->control);
		if (generator->fill_block != NULL)
		{
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
			pthread_mutex_lock(block->lock);
			generator->fill_block(generator, block);
			pthread_mutex_unlock(block->lock);
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
			block = block->next;
		}
	}

	return NULL;
}



struct packet_block *create_block_circle(int count, int block_len)
{
	struct packet_block *block = malloc(sizeof(struct packet_block));
	CHKALLOC(block);
	packet_block_init(block, block_len);
	struct packet_block *first = block;

	for (int i = 1; i < count; i++)
	{
		struct packet_block *next = malloc(sizeof(struct packet_block));
		CHKALLOC(next);
		packet_block_init(next, block_len);
		block->next = next;
		block = next;
	}

	block->next = first;

	return first;
}



int destroy_block_circle(struct packet_block *block)
{
	struct packet_block *next = block->next;
	block->next = NULL;
	struct packet_block *current = next;
	while (current != NULL)
	{
		next = current->next;
		packet_block_destroy(current); // TODO: error check
		free(current);
		current = next;
	}
}



generator_option *split_generator_args(char *args)
{
	if (args == NULL)
		return NULL;

	/* copy input string, because strtok_r modifies a string while
	 * splitting it */
	char *a = strdup(args);
	CHKALLOC(a);

	/* calculate the number of elements (comma separated) in the
	 * arguments string */
	int elements = 0;
	char *s = a;
	while (s != NULL)
	{
		s = strchr(s, ',');
		elements++;
		if (s != NULL)
			s = s + 1;
	}

	/* array to store name/value pairs */
	generator_option* options =
		calloc(elements + 1, sizeof(generator_option));
	CHKALLOC(options);
	/* Last element is set to NULL to make the end of the array
	 * detectable */
	options[elements].name = NULL;
	options[elements].value = NULL;

	/* split the arguments string */
	int i = 0;
	char *token = NULL;
	char *name = NULL;
	char *value = NULL;
	char *saveptr = NULL;
	char *saveptr2 = NULL;
	for (token = strtok_r(a, ",", &saveptr);
	     token != NULL;
	     token = strtok_r(NULL, ",", &saveptr))
	{
		name = strtok_r(token, "=", &saveptr2);
		value = strtok_r(NULL, "=", &saveptr2);

		/* store the name value pair */
		options[i].name = strdup(name);
		options[i].value = strdup(value);
		i++;
	}

	free(a);
	return options;
}



void free_generator_args(generator_option *args)
{
	if (args == NULL)
		return;

	for (int i = 0; args[i].name != NULL; i++)
	{
		free(args[i].name);
		free(args[i].value);
	}
	free(args);
}
