#ifndef __FTG_GAUSSIAN_GENERATOR_H__
#define __FTG_GAUSSIAN_GENERATOR_H__

#include "generator.h"



/* Prepare a static generator using the given data. This generator
 * will create packets with exactly the given size and interval
 * only. */
int gaussian_generator_create(generator_t *this, char *args);

#endif /* __FTG_GAUSSIAN_GENERATOR_H__ */
