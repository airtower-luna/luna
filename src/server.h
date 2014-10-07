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
#ifndef __LUNA_SERVER_H_
#define __LUNA_SERVER_H_

#include <netinet/in.h>

/* option flags for the server */
#define SERVER_IPV6_ONLY 1
#define SERVER_TSV_OUTPUT 2
#define SERVER_SIGTERM_EXIT 4

int run_server(struct addrinfo *const addr, const int flags,
	       const char *const datafile);

void term_server(int signum);

#endif /* __LUNA_SERVER_H_ */
