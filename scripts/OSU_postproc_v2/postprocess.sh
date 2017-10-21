#!/bin/bash

d=$1
fname=$2
rm -f $fname
cntr=1
tmp=`ls -1 $1/*.log`
if [ -z "$tmp" ]; then
    exit 0
fi
for i in `ls -1 $1/*.log`; do
    if [ "$cntr" = "1" ]; then
        cat $i | grep "#"  > $fname
        cat $i | grep -v "#" | awk '{ print $1"\t"$2 }' > ${fname}.tmp
    else
        cat $i | grep -v "#" | awk '{ print $2 }' > tmp.1
        paste ${fname}.tmp tmp.1 > tmp.2
        cat tmp.2 > ${fname}.tmp
        rm tmp.1 tmp.2
    fi
    cntr=$(($cntr + 1))
done

cat ${fname}.tmp >> $fname
rm ${fname}.tmp