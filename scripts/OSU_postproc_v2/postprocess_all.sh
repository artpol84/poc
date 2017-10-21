#!/bin/bash

function checkdir()
{
    if [ -z "$1" ] || [ ! -d "$1" ]; then
        echo "bad dir"
        exit 0
    fi
}

binaries="osu_bibw osu_bw osu_latency osu_mbw_mr osu_multi_lat"

checkdir "$1"
indir=$1
shift
outdir=$1
mkdir -p $outdir
shift
suffix=$1
if [ -z "$suffix" ]; then
    echo "bad suffix"
    exit 0
fi

for i in $binaries; do
    rm -Rf $outdir/$i
    mkdir $outdir/$i
    tmp=`ls $indir/${i}*.log 2>/dev/null`
    if [ -z "$tmp" ]; then
	continue
    fi
    mv $indir/${i}*.log $outdir/$i
    if [ "$i" != "osu_mbw_mr" ]; then
        ./postprocess.sh $outdir/$i $outdir/${i}_${suffix}.log
    else
        ./postprocess_mr.sh $outdir/$i $outdir/${i}_${suffix}.log
    fi
    ./reduce.sh $outdir/${i}_${suffix}.log > $outdir/${i}_${suffix}_red.log
done