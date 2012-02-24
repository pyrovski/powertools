#!/bin/bash
. /usr/local/tools/dotkit/init.sh


#
# gnu version
#
export COMPILER_SPECIFIC_FLAGS=
use mvapich2-gnu-debug-1.7
make clean
make
cd mpi
make clean
make
cd ..
mv lib/librapl.so lib/librapl.mvapich2-gnu-debug-1.7.so

#
# icc version
#

export COMPILER_SPECIFIC_FLAGS=" -diag-disable 177 -diag-disable 279 -diag-disable 181 "
use mvapich2-intel-debug-1.7
make clean
make
cd mpi
make clean
make
cd ..
mv lib/librapl.so lib/librapl.mvapich2-icc-debug-1.7.so

