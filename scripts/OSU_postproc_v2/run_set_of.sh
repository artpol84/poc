#!/bin/bash


pt2pt_dir=$1
if [ -z "$pt2pt_dir" ]; then
    echo "Need OSU pt2pt directory"
    exit 1
fi
shift
config_list=$1
if [ -z "$config_list" ]; then
    echo "Need the list of directories with different configurations"
    exit 1
fi
shift
count=$1
if [ -z "$count" ]; then
    echo "Need number of iterations"
    exit 1
fi
shift

for binary in `ls -1 "$pt2pt_dir"`; do
    for i in `seq 1 $count`; do
        for confdir in $config_list; do
            if [ ! -d "$confdir" ]; then
                echo "Error: confdir \"$confdir\" is not a directory. Abort"
                exit 1
            fi
            cd $confdir
            ./run_p2p_set.sh -bin=$pt2pt_dir/$binary -iter=$i $@
            cd ..
        done
    done
done