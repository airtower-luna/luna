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
 *
 * The generator MUST ensure that the generated packet sizes are not
 * below MIN_PACKET_SIZE (defined in fast-tg.h).
 */
typedef struct generator_t generator_t;
struct generator_t
{
	/* The "first" block of the ring buffer (set during init) */
	struct packet_block *block;
	/* Generator posts to this semaphore when its initialization
	 * is complete. */
	sem_t *ready;
	/* Semaphore to control dynamic data generation. The sending
	 * thread posts to this after using one block of data. */
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
	int (*fill_block)(generator_t *this, struct packet_block *current);
	/* This function must free all memory behind *arg and the list
	 * of blocks behind **block. */
	int (*destroy_generator)(generator_t *this);
};



/*
 * Struct to store command line arguments for the generator as name
 * value pairs
 */
typedef struct generator_option generator_option;
struct generator_option
{
	char *name;
	char *value;
};



/*
 * A generator struct and the function to create that type of generator
 *
 * The function MUST accept a NULL in generator_args and use default
 * values in that case.
 */
struct generator_type
{
	char *name;
	int (*create)(generator_t *generator, generator_option *generator_args);
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

/* Split an arguments string into a generator_option[]. The string is
 * expected in the format name=value,name2=value2,... In the last
 * element of the array both name and value are set to NULL to allow
 * detecting the end of the array. */
generator_option *split_generator_args(char *args);

/* Free a generator_option[], including the strings pointed to by the
 * elements. */
void free_generator_args(generator_option *args);

#endif /* __FTG_GENERATOR_H__ */
