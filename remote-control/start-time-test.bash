#!/bin/bash

# This file is part of the Lightweight Universal Network Analyzer (LUNA)
#
# Copyright (c) 2014 Fiona Klute
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
stdoutfile=$(mktemp)
luna_path=$(readlink -f "${BUILDDIR}/src/luna")

# write configuration file
echo -e "default_exec=${luna_path}" >${conffile}
cat - >>${conffile} << EOF
startup_delay=3
[conn]
client=localhost
server=localhost
generator=static
generator_args=size=50
EOF
echo "server_output=${outfile}" >>${conffile}

# run transmission, check result
ret=0
set -o pipefail
if ! ${BUILDDIR}/remote-control/luna-control ${conffile} | tee ${stdoutfile}
then
    ret=1
else
    # find start time in the log
    st=$(cat ${stdoutfile} | grep "General start time:" | cut -d' ' -f4)
    echo "Scheduled start time: ${st}"
    echo "First recorded packet:"
    # check if the first packet arrived in the correct second
    if head -2 ${outfile} | tail -1 - | grep -P "^${st}"; then
	echo "Seconds match!"
    else
	echo "Scheduled and recorded times do not match!"
	ret=1
    fi
fi

# cleanup
rm $conffile
rm $outfile
rm $stdoutfile
exit $ret
