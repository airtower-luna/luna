#include <config.h>

#include <stdio.h>

#include "fast-tg.h"
#include "traffic.h"

int packet_block_init(struct packet_block *block, int length)
{
	block->lock = malloc(sizeof(pthread_mutex_t));
	CHKALLOC(block->lock);
	pthread_mutex_init(block->lock, NULL);
	block->length = length;
	block->data = calloc(length, sizeof(struct packet_data));
	CHKALLOC(block->data);
	block->next = NULL;
	return 0;
}



int packet_block_destroy(struct packet_block *block)
{
	int ret = 0;
	if (pthread_mutex_destroy(block->lock) != 0)
	{
		perror("Mutex still locked while trying to free");
		ret = 1;
	}
	free(block->lock);
	free(block->data);
	return ret;
}
