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
#ifndef __LUNA_CLIENT_H__
#define __LUNA_CLIENT_H__

#include <netinet/in.h>

/*
 * addr: destination (IP address, port)
 * time: time (in seconds) to send packets
 * start_time: time when the client should start sending
 * clk_id: Clock to use for packet timing, start_time is compared to
 *	   this clock. See time.h for available clocks.
 * echo: request echo packets?
 * generator_type: name of the generator to use
 * generator_args: parameters for the generator
 */
int run_client(struct addrinfo *addr, int time,
	       struct timespec start_time, clockid_t clk_id,
	       int echo,
	       char *generator_type, char *generator_args,
	       const char *datafile);

#endif /* __LUNA_CLIENT_H__ */
