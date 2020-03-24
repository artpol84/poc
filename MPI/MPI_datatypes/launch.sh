#!/bin/bash

function exec_test()
{
    if [ -f "./$1" ]; then
        execs="./$1"
    else
        execs="./${1}_offs0 ./${1}_offsM"
    fi
    for e in $execs; do
        cmd="$MPIRUN -np 2 --map-by node --mca pml ucx -x UCX_TLS=rc_x --mca btl self $e"
        echo "Executing: $cmd"
        $cmd
    done
}

tests="vector index_plain index_regular_str index_regular_ilv index_ilv_w_block index_2x_ilv index_2x_str index_mixed1 index_two_dts"

for t in $tests; do
    exec_test $t
done

