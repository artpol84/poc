#!/bin/bash

orig_shift=1
lines=0
header=0


function grep_range_num()
{
    local in_file=$1
    local out_file=$2
    local range_num=$3
    local start=$(( $orig_shift + ( $header + $lines ) * $range_num + $header ))
    local end=$(( $start + $lines ))
    
    if [ ! -f $in_file ]; then
        echo 0
        return
    fi
    
    fsize=`cat $in_file | wc -l`
    if [ "$end" -gt "$fsize" ]; then
        echo "0"
        return
    fi
    
    head -n $end $in_file | tail -n $lines > $out_file
    echo "1"
}

function calc_header()
{
    local fname=$1
    local line_num=0
    local header_done=0
    
    while read line; do
        line_num=$(( line_num + 1 ))
        if [ "$line_num" -eq 1 ]; then
            continue
        fi
        
        tmp=`echo $line | grep "^#"`
        if [ $header_done = "0" ] && [ -n "$tmp" ]; then
            header=$(( $header + 1 ))
        elif [ -z "$tmp" ]; then
            lines=$(( $lines + 1 ))
            header_done="1"
        else
            break
        fi
    done < $fname
}

tdir=./postprocess_tmp
cleared=$tdir/cleared
data=$tdir/data
tmp1=$tdir/tmp1
tmp2=$tdir/tmp2

mkdir -p $tdir

cat $1 | grep -v "^thread" | grep -v "^main" | grep -v "^rank" > $cleared

out_dir=`dirname $1`
tmp_fname=`basename $1`
out_file_prefix=${tmp_fname%%.xls}

calc_header $cleared

ret="1"
i=0
while [ "$ret" = "1" ]; do
    ret=`grep_range_num $cleared $data.$i $i`
    if [ "$ret" = "1" ]; then
        i=$(( $i + 1 ))
    fi
done

if [ "$i" -eq "0" ]; then
    echo "Not enough data in the $1"
    exit 0
fi
count=$(( $i - 1 ))

fields=`cat $data.0 | head -n 1 | awk '{ print NF }'`

for i in `seq 2 $fields`; do
    out_file=$out_dir/${out_file_prefix}_f$i.xls
    cat $data.0 | awk '{ print $1 }' > $out_file
    
    for j in `seq 0 $count`; do
        format="{ print \$$i }"
        cat $data.$j | awk "$format" > $tmp2
        cp $out_file $tmp1
        paste $tmp1 $tmp2 > $out_file
    done
done
