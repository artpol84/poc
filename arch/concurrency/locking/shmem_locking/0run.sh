#!/bin/bash

module load hpcx-gcc

# NOTE: Use of rankfile helps with locality here. Without it a high level of perf 'cache-misses' was observed

test=$1
echo "Test = $test"

header=""

get_header()
{
    header=`mpirun -np 2 $test | head -n1`    
}

execute_test()
{
    np=$1

    cum_wo_oprate=0
    cum_ro_oprate=0
    cum_rw_wlck_lat=0
    cum_rw_rlck_oprate=0
    
    echo "$header"
    for i in `seq 1 10`; do
        out=`mpirun -np $np --bind-to core --rankfile rankfile $test | tail -n1`
        echo $out
        wo_oprate=`echo $out | awk '{ print $3 }'`
        ro_oprate=`echo $out | awk '{ print $5 }'`
        rw_wlck_lat=`echo $out | awk '{ print $6 }'`
        rw_rlck_oprate=`echo $out | awk '{ print $8 }' | awk -F "/"  '{ print $1}'`

        cum_wo_oprate=`echo "( $cum_wo_oprate + $wo_oprate )" | bc`
        cum_ro_oprate=`echo "( $cum_ro_oprate + $ro_oprate )" | bc`
        cum_rw_wlck_lat=`echo "( $cum_rw_wlck_lat + $rw_wlck_lat )" | bc`
        cum_rw_rlck_oprate=`echo "( $cum_rw_rlck_oprate + $rw_rlck_oprate )" | bc`
    done

    cum_wo_oprate=`echo "( $cum_wo_oprate / 10 )" | bc`
    cum_ro_oprate=`echo "( $cum_ro_oprate / 10 )" | bc`

    cum_rw_wlck_lat=`echo "scale=10; ( $cum_rw_wlck_lat / 10.0 )" | bc`
    cum_rw_rlck_oprate=`echo "( $cum_rw_rlck_oprate / 10 )" | bc`
    
    echo "$(( $np - 1))  $cum_wo_oprate $cum_ro_oprate $cum_rw_wlck_lat $cum_rw_rlck_oprate"
    echo "$(( $np - 1)) $cum_wo_oprate $cum_ro_oprate $cum_rw_wlck_lat $cum_rw_rlck_oprate" >> `basename $test`_summary.log
}

get_header

for proc in 2 3 5 9 17 28; do
    echo " ============= $proc ============= "
    execute_test $proc
done
