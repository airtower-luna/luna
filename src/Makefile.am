# This file is part of the Lightweight Universal Network Analyzer (LUNA)
#
# Copyright (c) 2013 Fiona Klute
#
# LUNA is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# LUNA is distributed in the hope that it will be useful, but WITHOUT
# ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
# or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public
# License for more details.
#
# You should have received a copy of the GNU General Public License
# along with LUNA. If not, see <http://www.gnu.org/licenses/>.

# Build the LUNA binary
bin_PROGRAMS = luna
luna_SOURCES = luna.c server.c traffic.c generator.c gaussian_generator.c \
	simple_generator.c client.c
# header files don't need to be installed (because there's no library),
# but must be included in source packages
noinst_HEADERS = client.h gaussian_generator.h generator.h luna.h server.h \
	simple_generator.h traffic.h

LIBS = $(LIBRT) $(LIBGSL_LIBS)

# manpages
dist_man1_MANS = luna.man

install-exec-hook:
	if ($(SET_CAPS)); then \
	setcap cap_sys_nice,cap_ipc_lock=pe $(DESTDIR)$(bindir)/luna || \
	echo "WARNING: Could not set capabilities for LUNA executable." >&2; \
	fi
