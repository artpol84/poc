#!/bin/bash

d1=$1
d2=$2

binaries="osu_bibw osu_bw osu_latency osu_mbw_mr osu_multi_lat"

for i in $binaries; do
    f1=`ls $d1/${i}*_red.log`
    f2=`ls $d2/${i}*_red.log`
    echo "----------- $i ----------- "
    if [ "$i" != "osu_latency" ] && [ "$i" != "osu_multi_lat" ]; then
        paste $f1 $f2 | awk '{ i= 100 * ($4 - $2) / $2; printf("%u\t%0.3f\t%0.3f\t%0.2f%%\n", $1, $2, $4, i); }'
    else
        paste $f1 $f2 | awk '{ i= 100 * ($2 - $4) / $2; printf("%u\t%0.3f\t%0.3f\t%0.2f%%\n", $1, $2, $4, i); }'
    fi
done
