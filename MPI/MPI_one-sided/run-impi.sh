#!/bin/bash

NP=$1
OMP=$2

set -x
mpiicc -o win_test.bin -qopenmp -g win_test.c

hcoll="-mca coll_hcoll_enable 0 "

mpirun -n $NP -genv OMP_NUM_THREADS=$OMP -genv I_MPI_PIN_DOMAIN=omp ./win_test.bin $3

set +x
