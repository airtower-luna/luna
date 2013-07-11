#include <config.h>

#include <string.h>

#include "fast-tg.h"
#include "static_generator.h"

/* TODO: meaningful block length */
#define BLOCK_LEN 10

int static_generator_init(generator_t *this);
int static_generator_destroy(generator_t *this);

int alternate_time_generator_init(generator_t *this);

struct static_generator_attr
{
	int size;
	struct timespec interval;
};



/* Helper function for the creation of simple generators that treat
 * size as a maximum. The interval can be used as is, but can also be
 * modified without breaking anything. */
int simple_generator_base(generator_t *this,
			  int size, struct timespec *interval)
{
	this->max_size = size;

	this->attr = malloc(sizeof(struct static_generator_attr));
	CHKALLOC(this->attr);
	struct static_generator_attr *attr =
		(struct static_generator_attr *) this->attr;
	attr->size = size;
	memcpy(&(attr->interval), interval, sizeof(struct timespec));
}



int static_generator_create(generator_t *this,
			    int size, struct timespec *interval)
{
	this->init_generator = &static_generator_init;
	this->generate_block = NULL;
	this->destroy_generator = &static_generator_destroy;

	simple_generator_base(this, size, interval);
}



int static_generator_init(generator_t *this)
{
	struct static_generator_attr *attr =
		(struct static_generator_attr *) this->attr;

	*(this->block) = create_block_circle(1, BLOCK_LEN);

	struct packet_block *block = *(this->block);
	for (int i = 0; i < block->length; i++)
	{
		block->data[i].size = attr->size;
		memcpy(&(block->data[i].delay), &(attr->interval),
		       sizeof(struct timespec));
	}

	return 0;
}



int static_generator_destroy(generator_t *this)
{
	destroy_block_circle(*(this->block)); // TODO: error check
	*(this->block) = NULL;
	free(this->attr);
}
