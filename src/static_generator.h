#ifndef __FTG_STATIC_GENERATOR_H__
#define __FTG_STATIC_GENERATOR_H__

#include <pthread.h>
#include <time.h>

#include "generator.h"
#include "traffic.h"



/* Prepare a static generator using the given data. This generator
 * will create packets with exactly the given size and interval
 * only. */
int static_generator_create(generator_t *this,
			    int size, struct timespec *interval);

/* Prepare a random size generator using the given data. This
 * generator will randomly change packet sizes block-wise, using the
 * given size as the maximum size. */
int rand_size_generator_create(generator_t *this,
			       int size, struct timespec *interval);

#endif /* __FTG_STATIC_GENERATOR_H__ */
