#ifndef __LUNA_GAUSSIAN_GENERATOR_H__
#define __LUNA_GAUSSIAN_GENERATOR_H__

#include "generator.h"



/* Prepare a static generator using the given data. This generator
 * will create packets with exactly the given size and interval
 * only. */
int gaussian_generator_create(generator_t *this, generator_option *args);

#endif /* __LUNA_GAUSSIAN_GENERATOR_H__ */
