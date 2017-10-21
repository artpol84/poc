#!/bin/bash

file=$1

cat $file | grep -v "#" > $file.tmp

while read -r line
do
    count=0
    sum=0
    size=0
    for j in $line; do
        if [ "$count" = "0" ]; then
            count=$(( $count + 1))
            sz=$j
            continue
        fi
        count=$(( $count + 1))
        sum=`echo "scale=3; $sum + $j" | bc`
    done
    avg=`echo "scale=3; $sum / ($count - 1)" | bc`
    echo -e "$sz\t$avg"
done < $file.tmp

rm $file.tmp