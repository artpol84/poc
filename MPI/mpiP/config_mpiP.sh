#!/bin/bash

LUNWIND=/home/artpol/sandbox/libunwind-1.3.1/
MPI_BASE=/home/artpol/sandbox/openmpi-3.1.3

./configure --prefix=`pwd`/install \
			--with-binutils-dir=/home/artpol/sandbox/binutils-2.32/ \
			--with-libunwind=$LUNWIND \
			--with-cc=$MPI_BASE/bin/mpicc \
			--without-f77