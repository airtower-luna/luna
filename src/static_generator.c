#include <config.h>

#include <string.h>

#include "fast-tg.h"
#include "static_generator.h"

int static_generator_init(generator_t *this);
int static_generator_destroy(generator_t *this);

struct static_generator_attr
{
	int size;
	struct timespec interval;
};



int static_generator_create(generator_t *this,
			    int size, struct timespec *interval)
{
	this->init_generator = &static_generator_init;
	this->generate_block = NULL;
	this->destroy_generator = &static_generator_destroy;

	this->max_size = size;

	this->attr = malloc(sizeof(struct static_generator_attr));
	CHKALLOC(this->attr);
	struct static_generator_attr *attr =
		(struct static_generator_attr *) this->attr;
	attr->size = size;
	memcpy(&(attr->interval), interval, sizeof(struct timespec));
}



int static_generator_init(generator_t *this)
{
	struct static_generator_attr *attr =
		(struct static_generator_attr *) this->attr;

	/* TODO: meaningful block length */
#define BLOCK_LEN 10
	struct packet_block *block = malloc(sizeof(struct packet_block));
	CHKALLOC(block);
	packet_block_init(block, BLOCK_LEN);
	block->next = block;

	for (int i = 0; i < block->length; i++)
	{
		block->data[i].size = attr->size;
		memcpy(&(block->data[i].delay), &(attr->interval),
		       sizeof(struct timespec));
	}
	*(this->block) = block;

	return 0;
}



int static_generator_destroy(generator_t *this)
{
	packet_block_destroy(*(this->block)); // TODO: error check
	free(*(this->block));
	*(this->block) = NULL;
	free(this->attr);
}
