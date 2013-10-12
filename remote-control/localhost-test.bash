#!/bin/bash

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
