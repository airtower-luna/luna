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
data_DATA = luna-eval.m
# Evaluation scripts are installed to libexec
pkglibexec_SCRIPTS = timediff rtt datarate lte-analysis sizedist
CLEANFILES = $(pkglibexec_SCRIPTS)
# TODO: general evaluation script that calls the ones from libexec

# replace path to luna-eval.m with the post-install one
do_subst = sed -e 's,"luna-eval.m","$(datadir)/luna-eval.m",g'

$(pkglibexec_SCRIPTS): %: %.m Makefile
	$(do_subst) < $< > $@
	chmod +x $@
