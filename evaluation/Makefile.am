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

# Install the LUNA evaluation helper functions file
dist_data_DATA = luna-eval.m
# Evaluation scripts are installed to libexec
pkglibexec_SCRIPTS = timediff rtt datarate lte-analysis sizedist
# Master evaluation script (for post-install use)
bin_SCRIPTS = luna-eval
EXTRA_DIST = luna-eval.in timediff.m rtt.m datarate.m lte-analysis.m sizedist.m

# clean modified scripts
CLEANFILES = $(bin_SCRIPTS) $(pkglibexec_SCRIPTS)

# set path to scripts installed to $pkglibexecdir
luna-eval: luna-eval.in Makefile
	sed -e 's,LIBEXECPATH="",LIBEXECPATH="$(pkglibexecdir)",g' < $< > $@
	chmod +x $@

# replace path to luna-eval.m with the post-install one
$(pkglibexec_SCRIPTS): %: %.m Makefile
	sed -e 's,"luna-eval.m","$(datadir)/luna-eval.m",g' < $< > $@
	chmod +x $@
