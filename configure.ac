# This file is part of the Fast Traffic Generator (fast-tg)
#
# Copyright (c) 2013 Fiona Klute
#
# fast-tg is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# fast-tg is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with fast-tg. If not, see <http://www.gnu.org/licenses/>.

# -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ([2.65])
AC_INIT([fast-tg], [0.1], [fiona2.klute@uni-dortmund.de])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([src/fast-tg.c])
AC_CONFIG_HEADERS([config.h])

# Checks for programs.
AC_PROG_CC_C99
AC_PROG_INSTALL

# Checks for libraries.
AC_GNU_SOURCE
LIBRT=""
AC_CHECK_LIB(rt, clock_getres, [LIBRT="-lrt"])
AC_SUBST(LIBRT)
#PKG_CHECK_MODULES(GLIB, glib-2.0)
#AC_SUBST(GLIB_CFLAGS)
#AC_SUBST(GLIB_LIBS)

# Checks for header files.
AC_CHECK_HEADERS([stdlib.h signal.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T

# Stuff to build
AC_CONFIG_FILES([Makefile src/Makefile])
AC_OUTPUT
