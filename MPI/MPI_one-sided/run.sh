#!/bin/bash

test=$1
NP=$2
OMP=$3

set -x
mpicc -o ${test}.bin -g -fopenmp ${test}.c

hcoll="-mca coll_hcoll_enable 0 "

mpirun -np $NP --report-bindings --bind-to socket --map-by socket -mca osc ucx -mca btl ^openib -x OMP_NUM_THREADS=$OMP -mca opal_warn_on_missing_libcuda 0 --tag-output $hcoll ./${test}.bin $4

set +x
