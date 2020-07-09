#!/bin/bash

module load hpcx-gcc 

verbose=0

if [ $1 = "--verbose" ]; then
    verbose=1
    shift 1
fi

procs=$1
map_by=$2
binary=$3
shift 3

if [ "$output" = 1 ]; then
    echo -n "mpirun -np $procs --mca pml ^yalla,ucx" > ${3}_${1}_${2}.log 
    echo -n " --mca coll_hcoll_enable 0" >> ${3}_${1}_${2}.log 
    echo -n " --mca btl self,vader" >> ${3}_${1}_${2}.log 
    echo -n " --mca mtl ^mxm" >> ${3}_${1}_${2}.log 
    echo -n " --bind-to hwthread" >> ${3}_${1}_${2}.log 
    echo -n " --map-by $map_by" >> ${3}_${1}_${2}.log 
    echo -n " --report-bindings" >> ${3}_${1}_${2}.log 
    echo    " ./$binary" >> ${3}_${1}_${2}.log 
fi
shift 3

outfile=${binary}_${procs}_${map_by}.log 
rep_bindings="--report-bindings"
if [ "$verbose" = 0 ]; then
    outfile=/dev/null
    rep_bindings=""
fi

mpirun -np $procs --mca pml ^yalla,ucx \
            --mca coll_hcoll_enable 0 \
            --mca btl self,vader \
            --mca mtl ^mxm \
            --bind-to hwthread \
            --map-by $map_by \
            $rep_bindings \
            ./$binary $@ 2>&1 | tee -a $outfile
