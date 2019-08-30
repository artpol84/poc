#!/bin/bash

#UCX_BASE=/hpc/local/benchmarks/daily/next/2019-08-29/hpcx-gcc-redhat7.4/ucx/prof/lib/
UCX_BASE=/labhome/artemp/mtr_scrap/UCX/2019_07_31_UCM_mt/ucx-1.7.0/install/lib/

LD_PRELOAD="$UCX_BASE/libucp.so:$UCX_BASE/libuct.so:$UCX_BASE/libucm.so:$UCX_BASE/libucs.so"
module load hpcx-gcc
mpirun -np $1 --rankfile ./rankfile -mca coll ^hcoll -x LD_PRELOAD=$LD_PRELOAD  ./osu_mbw_mr_dummy -m 1:1 -W 64  -i 100 #0000000

#--bind-to core 