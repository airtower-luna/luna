#include <config.h>

#include <stdio.h>

#include "fast-tg.h"
#include "generator.h"

void* run_generator(void *arg)
{
	struct generator_t *generator = (struct generator_t *) arg;

	/* TODO: set realtime priority */

	generator->init_generator(generator);
	struct packet_block *block = *(generator->block);

	sem_post(generator->ready);

	/* this loop will be stopped by thread cancellation */
	while (1)
	{
		sem_wait(generator->control);
		if (generator->generate_block != NULL)
		{
			pthread_setcancelstate(PTHREAD_CANCEL_DISABLE, NULL);
			pthread_mutex_lock(block->lock);
			generator->generate_block(generator, block);
			pthread_mutex_unlock(block->lock);
			pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
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
