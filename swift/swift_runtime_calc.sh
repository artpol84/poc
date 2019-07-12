#!/bin/bash

max_step=`cat $1 |  grep -P "^\ .\ *[0-9].[0-9]*" | tail -n1 | awk '{print $1}'`
wtime=`cat $1 |  grep -P "^\ .\ *[0-9].[0-9]*" | awk 'BEGIN{tot=0} {tot += $12} END {print tot}'`



echo "$wtime $max_step" | awk '{printf "wall clock: " ($1 / 1000) " sec; perf: " (($2 * 3600000) / $1) "\n" }'