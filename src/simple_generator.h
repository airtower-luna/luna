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
#ifndef __LUNA_SIMPLE_GENERATOR_H__
#define __LUNA_SIMPLE_GENERATOR_H__

#include "generator.h"



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

#endif /* __LUNA_SIMPLE_GENERATOR_H__ */
