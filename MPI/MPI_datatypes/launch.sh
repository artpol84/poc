#!/bin/bash

function exec_test()
{
    cmd="$MPIRUN -np 2 --map-by node --mca pml ucx -x UCX_TLS=rc_x --mca btl self ./$1"
    echo "Executing: $cmd"
    $cmd
}

tests="vector index_plain index_regular index_regular_ilv index_ilv_w_block index_2x_ilv index_2x_stride index_mixed1"
for t in $tests; do
    exec_test $t
done

