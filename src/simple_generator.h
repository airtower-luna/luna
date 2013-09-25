#ifndef __FTG_SIMPLE_GENERATOR_H__
#define __FTG_SIMPLE_GENERATOR_H__

#include <pthread.h>
#include <time.h>

#include "generator.h"
#include "traffic.h"



/* Prepare a static generator using the given data. This generator
 * will create packets with exactly the given size and interval
 * only. */
int static_generator_create(generator_t *this, generator_option *args);

/* Prepare a random size generator using the given data. This
 * generator will randomly change packet sizes block-wise, using the
 * given size as the maximum size. */
int rand_size_generator_create(generator_t *this, generator_option *args);

/* Prepare an alternating time generator using the given data. All
 * packets will have the same size, but every second block will use
 * double the interval between them. */
int alternate_time_generator_create(generator_t *this, generator_option *args);

#endif /* __FTG_SIMPLE_GENERATOR_H__ */
