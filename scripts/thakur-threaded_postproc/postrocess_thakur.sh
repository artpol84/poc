#!/bin/bash

dir_name=$1
binding=$2
testname=$3
pml=$4
fprefix=${pml}_${binding}_${testname}_

function count_thread_vars()
{
    ls -1 $dir_name/${fprefix}* | wc -l
}



function process_bw()
{
    local fname=$1
    local thread_num=$2
    local exp_num=`cat $fname | grep "Bandwidth (Mbits/s)" | wc -l`
    
    numbers=`cat $fname | grep "^[0-9]"`
    _sum=0
    for i in $numbers; do
        _sum=`echo "${_sum} + $i" | bc `
    done
    result=`echo "${_sum} / $exp_num" | bc`
    echo $result
}

function process_mr()
{
    local fname=$1
    local thread_num=$2
    local exp_num=`cat $fname | grep "Thread [0-9]" | wc -l`
    
    exp_num=`echo "$exp_num / $thread_num" | bc`
    
    numbers=`cat $fname | grep "^Thread [0-9]" | awk '{ print $5 }'`
    _sum=0
    for i in $numbers; do
        _sum=`echo "${_sum} + $i" | bc `
    done
    result=`echo "${_sum} / $exp_num" | bc`
    echo $result
}

thr_num=`count_thread_vars`
for filename in `ls -1 $dir_name/${fprefix}*`; do
    bname=`basename $filename`
    tnum=`echo $bname | sed -e "s/$fprefix//" | awk -F "_" '{ print $1 }'`

    if [ "$testname" = "bw_th" ]; then
        echo "$tnum "`process_bw $filename $thr_num` 
    elif [ "$testname" = "message_rate_th" ]; then
        echo "$tnum "`process_mr $filename $thr_num` 
    fi
    i=$(( $i + 1))
done
