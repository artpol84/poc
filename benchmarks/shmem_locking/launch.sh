#!/bin/bash

module load hpcx-gcc 

echo -n "mpirun -np $1 --mca pml ^yalla,ucx" > ${3}_${1}_${2}.log 
echo -n " --mca coll_hcoll_enable 0" >> ${3}_${1}_${2}.log 
echo -n " --mca btl self,vader" >> ${3}_${1}_${2}.log 
echo -n " --mca mtl ^mxm" >> ${3}_${1}_${2}.log 
echo -n " --bind-to hwthread" >> ${3}_${1}_${2}.log 
echo -n " --map-by $2" >> ${3}_${1}_${2}.log 
echo -n " --report-bindings" >> ${3}_${1}_${2}.log 
echo    " ./$3" >> ${3}_${1}_${2}.log 

mpirun -np $1 --mca pml ^yalla,ucx \
            --mca coll_hcoll_enable 0 \
            --mca btl self,vader \
            --mca mtl ^mxm \
            --bind-to hwthread \
            --map-by $2 \
            --report-bindings \
            ./$3 2>&1 | tee -a ${3}_${1}_${2}.log 