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

# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.72])
AC_INIT([luna],[0.1],[fiona2.klute@uni-dortmund.de])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/luna.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC
AC_PROG_INSTALL

# Checks for libraries.
AC_USE_SYSTEM_EXTENSIONS
# realtime stuff
LIBRT=""
AC_CHECK_LIB(rt, clock_getres, [LIBRT="-lrt"])
AC_SUBST(LIBRT)
# libgsl
PKG_CHECK_MODULES(LIBGSL, gsl >= 1.15)
AC_SUBST(LIBGSL_LIBS)
AC_SUBST(LIBGSL_CFLAGS)

# enable pthreads
AX_PTHREAD
LIBS="$PTHREAD_LIBS $LIBS"
CFLAGS="$CFLAGS $PTHREAD_CFLAGS"
CC="$PTHREAD_CC"

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h signal.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

AC_ARG_ENABLE(kutime,
	[  --enable-kutime	Record user space times when receiving packages],
	[AC_DEFINE([ENABLE_KUTIME], [1], [Test])], [])
AC_ARG_ENABLE(capabilities,
	[  --enable-capabilities	Set file capabilities for LUNA binary
			during installation],
	[AC_SUBST([SET_CAPS], [true])], AC_SUBST([SET_CAPS], [false]))

# Stuff to build
AC_CONFIG_FILES([Makefile src/Makefile evaluation/Makefile \
			  remote-control/Makefile])
AC_OUTPUT
