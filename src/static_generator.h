#ifndef __FTG_STATIC_GENERATOR_H__
#define __FTG_STATIC_GENERATOR_H__

#include <pthread.h>
#include <time.h>

#include "generator.h"
#include "traffic.h"



/* Set up the function pointers */
int static_generator_create(generator_t *this,
			    int size, struct timespec *interval);

#endif /* __FTG_STATIC_GENERATOR_H__ */
