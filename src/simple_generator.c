#include <config.h>

#include <string.h>

#include "luna.h"
#include "simple_generator.h"

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



/*
 * Helper function for the creation of simple generators that treat
 * the size parameter as a maximum. The interval can be used as is,
 * but can also be modified without breaking anything. The parameters
 * are taken from args. If args is NULL, or a parameter is missing,
 * default values will be used.
 *
 * interval (i): time between two packets (µs)
 * size (s): packet size in bytes (must be at least 4)
 */
int simple_generator_base(generator_t *this, generator_option *args)
{
	int size = MIN_PACKET_SIZE;
	int interval = 1000;

	if (args != NULL)
	{
		for (int i = 0; args[i].name != NULL; i++)
		{
			char *name = args[i].name;
			char *value = args[i].value;
			if (strcmp(name, "size") == 0
			    || strcmp(name, "s") == 0)
				size = atoi(value);
			if (strcmp(name, "interval") == 0
			    || strcmp(name, "i") == 0)
				interval = atoi(value);
			// TODO: catch unknown params
		}
	}

	if (size < MIN_PACKET_SIZE)
		size = MIN_PACKET_SIZE;

	this->max_size = size;

	this->attr = malloc(sizeof(struct static_generator_attr));
	CHKALLOC(this->attr);
	struct static_generator_attr *attr =
		(struct static_generator_attr *) this->attr;
	attr->size = size;
	attr->interval.tv_sec = interval / US_PER_S;
	attr->interval.tv_nsec = (interval % US_PER_S) * 1000;
}



int static_generator_create(generator_t *this, generator_option *args)
{
	this->init_generator = &static_generator_init;
	this->fill_block = NULL;
	this->destroy_generator = &static_generator_destroy;

	simple_generator_base(this, args);
}



int alternate_time_generator_create(generator_t *this, generator_option *args)
{
	this->init_generator = &alternate_time_generator_init;
	this->fill_block = NULL;
	this->destroy_generator = &static_generator_destroy;

	simple_generator_base(this, args);
}



int rand_size_generator_create(generator_t *this, generator_option *args)
{
	this->init_generator = &rand_size_generator_init;
	this->fill_block = &rand_size_generator_fill_block;
	this->destroy_generator = &static_generator_destroy;

	simple_generator_base(this, args);
}



int static_generator_init(generator_t *this)
{
	struct static_generator_attr *attr =
		(struct static_generator_attr *) this->attr;

	this->block = create_block_circle(1, BLOCK_LEN);

	struct packet_block *block = this->block;
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

	this->block = create_block_circle(2, BLOCK_LEN);

	struct timespec alt_interval;
	alt_interval.tv_sec = attr->interval.tv_sec;
	alt_interval.tv_nsec = attr->interval.tv_nsec * 2;
	int normal = 1;

	struct packet_block *block = this->block;
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
	} while (block != this->block);

	return 0;
}



/* This generator uses a random packet size that is the same
 * throughout one block. attr->size is used as a maximum. */
int rand_size_generator_init(generator_t *this)
{
	struct static_generator_attr *attr =
		(struct static_generator_attr *) this->attr;

	this->block = create_block_circle(4, BLOCK_LEN);

	struct packet_block *block = this->block;
	do
	{
		rand_size_generator_fill_block(this, block);
		block = block->next;
	} while (block != this->block);

	return 0;
}



/* Refill the block using a new random number for packet
 * size. Intervals are set to attr->interval. */
int rand_size_generator_fill_block(generator_t *this,
				   struct packet_block *current)
{
	struct static_generator_attr *attr =
		(struct static_generator_attr *) this->attr;

	/* scale random number to range of MIN_PACKET_SIZE to attr->size */
	long int r = random() * (attr->size - MIN_PACKET_SIZE) / RAND_MAX
		+ MIN_PACKET_SIZE;

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
	int ret = destroy_block_circle(this->block);
	this->block = NULL;
	free(this->attr);
	return ret;
}
