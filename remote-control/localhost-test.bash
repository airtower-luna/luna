#!/bin/bash

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

# prepare files and binary path
conffile=$(mktemp)
outfile=$(mktemp)
luna_path=$(readlink -f "${BUILDDIR}/src/luna")

# write configuration file
echo -e "default_exec=${luna_path}" >${conffile}
cat - >>${conffile} << EOF
[conn]
client=localhost
server=localhost
generator=random_size
generator_args=size=50
EOF
echo "server_output=${outfile}" >>${conffile}

# run transmission, check result
ret=0
if ! ${BUILDDIR}/remote-control/luna-control ${conffile}; then
    ret=1
fi

# cleanup
rm $conffile
rm $outfile
exit $ret
