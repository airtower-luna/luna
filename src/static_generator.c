#include <config.h>

#include <string.h>

#include "fast-tg.h"
#include "static_generator.h"

/* TODO: meaningful block length */
#define BLOCK_LEN 10

int static_generator_init(generator_t *this);
int static_generator_destroy(generator_t *this);

int alternate_time_generator_init(generator_t *this);
int rand_size_generator_init(generator_t *this);
int rand_size_generator_fill_block(generator_t *this,
				   struct packet_block *current);

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
	this->fill_block = NULL;
	this->destroy_generator = &static_generator_destroy;

	simple_generator_base(this, size, interval);
}



int rand_size_generator_create(generator_t *this,
			       int size, struct timespec *interval)
{
	this->init_generator = &rand_size_generator_init;
	this->fill_block = &rand_size_generator_fill_block;
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



/* This generator provides two static blocks with the configured
 * packet size, but one will have doubled intervals. */
int alternate_time_generator_init(generator_t *this)
{
	struct static_generator_attr *attr =
		(struct static_generator_attr *) this->attr;

	*(this->block) = create_block_circle(2, BLOCK_LEN);

	struct timespec alt_interval;
	alt_interval.tv_sec = attr->interval.tv_sec;
	alt_interval.tv_nsec = attr->interval.tv_nsec * 2;
	int normal = 1;

	struct packet_block *block = *(this->block);
	do
	{
		for (int i = 0; i < block->length; i++)
		{
			block->data[i].size = attr->size;
			if (normal)
				memcpy(&(block->data[i].delay),
				       &(attr->interval),
				       sizeof(struct timespec));
			else
				memcpy(&(block->data[i].delay),
				       &(alt_interval),
				       sizeof(struct timespec));
		}
		block = block->next;
		normal = !normal;
	} while (block != *(this->block));

	return 0;
}



/* This generator uses a random packet size that is the same
 * throughout one block. attr->size is used as a maximum. */
int rand_size_generator_init(generator_t *this)
{
	struct static_generator_attr *attr =
		(struct static_generator_attr *) this->attr;

	*(this->block) = create_block_circle(4, BLOCK_LEN);

	struct packet_block *block = *(this->block);
	do
	{
		rand_size_generator_fill_block(this, block);
		block = block->next;
	} while (block != *(this->block));

	return 0;
}



/* Refill the block using a new random number for packet
 * size. Intervals are set to attr->interval. */
int rand_size_generator_fill_block(generator_t *this,
				   struct packet_block *current)
{
	struct static_generator_attr *attr =
		(struct static_generator_attr *) this->attr;

	long int r = random() * attr->size / RAND_MAX;
	if (r < MIN_PACKET_SIZE)
		r = MIN_PACKET_SIZE;
	for (int i = 0; i < current->length; i++)
	{
		current->data[i].size = r;
		memcpy(&(current->data[i].delay), &(attr->interval),
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
