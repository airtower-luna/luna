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

dist_bin_SCRIPTS = luna-control
CLEANFILES = $(dist_bin_SCRIPTS)
EXTRA_DIST = luna-control.pl localhost-test.bash
TESTS = localhost-test.bash
TESTS_ENVIRONMENT = export BUILDDIR=$(top_builddir);

luna-control: luna-control.pl Makefile
	sed -e 's,/usr/local/bin,$(bindir),g' < $< > $@
	chmod +x $@
