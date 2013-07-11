#ifndef __FTG_GENERATOR_H__
#define __FTG_GENERATOR_H__

#include <pthread.h>
#include <semaphore.h>

#include "traffic.h"



/*
 * Protype of a generic generator
 *
 * The functions must be thread-safe, but the generic generator code
 * will take care of thread management, including aquiring mutexes and
 * semaphore signalling.
 */
typedef struct generator_t generator_t;
struct generator_t
{
	/* The generator must place a pointer to the first block here
	 * during init. */
	struct packet_block **block;
	/* Semaphore for communcation between generator and sending
	 * thread */
	sem_t *control;
	/* maximum packet size */
	int max_size;
	/* Custom attributes (depends on the individual generator
	 * type) */
	void *attr;
	/* This function must initialize the block list, fill it with
	 * data, and set max_size */
	int (*init_generator)(generator_t *this);
	/* Fill the block at *current with fresh data, if this
	 * generator does dynamic generation. May be NULL
	 * otherwise. */
	int (*generate_block)(generator_t *this, struct packet_block *current);
	/* This function must free all memory behind *arg and the list
	 * of blocks behind **block. */
	int (*destroy_generator)(generator_t *this);
};



/*
 * Run the generator
 *
 * This function is meant to run as a dedicated generator thread,
 * using a generator_t* as the only parameter.
 */
void* run_generator(void *arg);



/* Create a circular buffer of count packet blocks, with block_len
 * elements each. Returns a pointer to the first block. */
struct packet_block *create_block_circle(int count, int block_len);

/* Destry a circular buffer of count packet blocks. The function
 * follows the next pointers to delete all elements of the circle. */
int destroy_block_circle(struct packet_block *block);

#endif /* __FTG_GENERATOR_H__ */
