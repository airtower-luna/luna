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

SUBDIRS	= src remote-control evaluation

# Remove $(pkglibexecdir) on uninstall, which currently is the only
# package-specific subdirectory used by LUNA. The option
# --ignore-fail-on-non-empty prevents error messages if the user has
# added anything (most likely evaluation scripts) independently of the
# LUNA installation.
uninstall-hook:
	rmdir --ignore-fail-on-non-empty $(pkglibexecdir) || true
