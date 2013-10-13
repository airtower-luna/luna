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

#include <math.h>
#include <string.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_randist.h>

#include "luna.h"
#include "gaussian_generator.h"

/* TODO: meaningful block length */
#define BLOCK_LEN 10

int gaussian_generator_init(generator_t *this);
int gaussian_generator_fill_block(generator_t *this,
				  struct packet_block *current);
int gaussian_generator_destroy(generator_t *this);

struct gaussian_generator_attr
{
	size_t max;
	size_t mean;
	double sigma;
	struct timespec interval;
	const gsl_rng_type *rng_type;
	gsl_rng *rng;
};



/*
 * Helper function for the creation of a gaussian generators. The
 * parameters must be contained in args. If args is NULL, or a
 * parameter is missing, default values will be used.
 *
 * interval (i): time between two packets (µs)
 * max (m): maximum packet size in bytes (must be at least 4)
 * sigma (s): standard deviation of packet size in bytes (double)
 */
int gaussian_generator_base(generator_t *this, generator_option *args)
{
	this->attr = malloc(sizeof(struct gaussian_generator_attr));
	CHKALLOC(this->attr);
	struct gaussian_generator_attr *attr =
		(struct gaussian_generator_attr *) this->attr;

	attr->max = 4 * MIN_PACKET_SIZE;
	attr->sigma = -1.0; /* negative value is used later to detect init */
	int interval = 1000;
	gsl_rng_type *rng_type = NULL;
	gsl_rng *rng = NULL;

	if (args != NULL)
	{
		for (int i = 0; args[i].name != NULL; i++)
		{
			char *name = args[i].name;
			char *value = args[i].value;
			if (strcmp(name, "max") == 0
			    || strcmp(name, "m") == 0)
				attr->max = atoi(value);
			if (strcmp(name, "sigma") == 0
			    || strcmp(name, "s") == 0)
				attr->sigma = atof(value);
			if (strcmp(name, "interval") == 0
			    || strcmp(name, "i") == 0)
				interval = atoi(value);
			// TODO: catch unknown params
		}
	}

	if (attr->max < MIN_PACKET_SIZE)
		attr->max = MIN_PACKET_SIZE;
	attr->mean = attr->max / 2;
	if (attr->sigma <= 0.0)
		attr->sigma = attr->mean / 3;

	this->max_size = attr->max;

	attr->interval.tv_sec = interval / US_PER_S;
	attr->interval.tv_nsec = (interval % US_PER_S) * 1000;
}



int gaussian_generator_create(generator_t *this, generator_option *args)
{
	this->init_generator = &gaussian_generator_init;
	this->fill_block = &gaussian_generator_fill_block;
	this->destroy_generator = &gaussian_generator_destroy;

	gaussian_generator_base(this, args);

	struct gaussian_generator_attr *attr =
		(struct gaussian_generator_attr *) this->attr;
	gsl_rng_env_setup();
	attr->rng_type = gsl_rng_default;
	attr->rng = gsl_rng_alloc(attr->rng_type);
}



int gaussian_generator_init(generator_t *this)
{
	struct gaussian_generator_attr *attr =
		(struct gaussian_generator_attr *) this->attr;

	this->block = create_block_circle(4, BLOCK_LEN);

	struct packet_block *block = this->block;
	do
	{
		gaussian_generator_fill_block(this, block);
		block = block->next;
	} while (block != this->block);

	return 0;
}



/* Refill the block using a new random number for packet
 * size. Intervals are set to attr->interval. */
int gaussian_generator_fill_block(generator_t *this,
				   struct packet_block *current)
{
	struct gaussian_generator_attr *attr =
		(struct gaussian_generator_attr *) this->attr;

	for (int i = 0; i < current->length; i++)
	{
		double d = gsl_ran_gaussian_ziggurat(attr->rng, attr->sigma);
		size_t r = lround(d);
		r += attr->mean;
		if (r < MIN_PACKET_SIZE)
			r = MIN_PACKET_SIZE;
		if (r > attr->max)
			r = attr->max;
		current->data[i].size = r;
		memcpy(&(current->data[i].delay), &(attr->interval),
		       sizeof(struct timespec));
	}

	return 0;
}



int gaussian_generator_destroy(generator_t *this)
{
	struct gaussian_generator_attr *attr =
		(struct gaussian_generator_attr *) this->attr;
	int ret = 0;

	ret = destroy_block_circle(this->block); // pass error, if any
	this->block = NULL;
	gsl_rng_free(attr->rng);
	free(this->attr);
	return ret;
}
