#!/bin/bash

max_thr=$1
niter=10

for tst in "fake" "cas" "swap"; do
    echo "Test: $tst"
    thr=1;
    while [ "$thr" -le "$max_thr" ]; do 
        # echo "test = $t"
        cum=0
        for i in `seq 1 $niter`; do 
            line=`./tslist_${tst} -n $thr -a 100000 | grep Performance`
            # echo $line
            val=`echo $line | awk '{ print $2 }'`
            cum=`echo "$val + $cum" | bc`
            # echo "cum=$cum"
        done
        avg=`echo "scale=3; $cum / $niter" | bc`
        # echo "avg=$avg"

        echo "$thr : avg_perf $avg"

        new_thr=$(( $thr * 2 ))
        if [ "$thr" -lt "$max_thr" ] && [ "$new_thr" -gt "$max_thr" ]; then
            thr=$max_thr
        else
            thr=$new_thr
        fi
    done

done