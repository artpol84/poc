#!/bin/bash

#tests_suite=$1
tests_suite="test_nblock"
#tests_suite="test_nblock test_mcs test_2nmutex test_1nmutex+1nsignal"
max_procs=8
procs_step=2
launch_num=10
header=""
launch=""

check_tests()
{
    for test in $tests_suite; do
        if [ ! -f $test ] ; then
            `make $test`
        fi
    done
}

get_header()
{
    test=`echo $tests_suite | awk '{ print $1 }'`
    launch=`echo "./$test"`
    header=`mpirun -np 2 $launch | head -n1`    
    echo "$header Test"
}

execute_test()
{
    np=$1
    for test in $tests_suite; do
        launch=`echo "./$test"`
        for i in `seq 1 $launch_num`; do
            out=`mpirun -np $np --bind-to core $launch | tail -n1`
            echo "$out $test"
        done
    done
}

check_tests
get_header

for ((proc=2; proc<=$max_procs; )); do
#    echo $proc
    execute_test $proc
    proc=`echo "$proc + $procs_step" | bc`
done