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
#ifndef __LUNA_TRAFFIC_H__
#define __LUNA_TRAFFIC_H__

#include <pthread.h>
#include <time.h>

/* Contains the information needed to send one packet */
struct packet_data
{
	struct timespec delay; /* delay relative to the previous packet */
	size_t size; /* UDP payload size, including LUNA header */
};

/* A lockable list of struct packet_data elements, with a pointer to
 * the next block to use */
struct packet_block
{
	/* Mutex must be used when separate threads are
	 * creating/sending data */
	pthread_mutex_t *lock;
	/* Length of the list at *data (in struct packet_data
	 * sizes) */
	int length;
	/* Pointer to the list of actual packet data sets */
	struct packet_data *data;
	/* The struct packet_block to use after this one */
	struct packet_block *next;
};

/* Initialize the block. Memory for *block must have been allocated by
 * the caller. This function sets length to the given length,
 * allocates memory for *lock and *data, and initializes the
 * mutex. *next is set to NULL. */
int packet_block_init(struct packet_block *block, int length);
/* Free all resources allocated by the block, but not the struct
 * itself. */
int packet_block_destroy(struct packet_block *block);

#endif /* __LUNA_TRAFFIC_H__ */
