#include <config.h>

#include "fast-tg.h"
#include "generator.h"

void* run_generator(void *arg)
{
	struct generator_t *generator = (struct generator_t *) arg;

	/* TODO: set realtime priority */

	generator->init_generator(generator);

	sem_post(generator->control);

	/* TODO: implement dynamic generation */

	return NULL;
}
