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

#include <stdio.h>

#include "luna.h"
#include "traffic.h"

int packet_block_init(struct packet_block *block, int length)
{
	block->lock = malloc(sizeof(pthread_mutex_t));
	CHKALLOC(block->lock);
	pthread_mutex_init(block->lock, NULL);
	block->length = length;
	block->data = calloc(length, sizeof(struct packet_data));
	CHKALLOC(block->data);
	block->next = NULL;
	return 0;
}



int packet_block_destroy(struct packet_block *block)
{
	int ret = 0;
	if (pthread_mutex_destroy(block->lock) != 0)
	{
		fprintf(stderr, "Mutex still locked while trying to free!\n");
		ret = 1;
	}
	free(block->lock);
	free(block->data);
	return ret;
}
