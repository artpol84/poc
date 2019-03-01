#!/bin/bash

for i in 10 20 40 80 160; do 
    append=""
    if [ "$i" != 10 ]; then
        append="--no-hdr"
    fi
    ./launch.sh $i $@ $append;
done