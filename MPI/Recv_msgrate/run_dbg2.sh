#!/bin/bash

#UCX_BASE=/hpc/local/benchmarks/daily/next/2019-08-29/hpcx-gcc-redhat7.4/ucx/prof/lib/
UCX_BASE=/labhome/artemp/mtr_scrap/UCX/2019_07_31_UCM_mt/ucx/install/lib/

LD_PRELOAD="$UCX_BASE/libucp.so:$UCX_BASE/libuct.so:$UCX_BASE/libucm.so:$UCX_BASE/libucs.so"
export LD_LIBRARY_PATH=$LD_LIBRARY_PATH:`pwd`
module load hpcx-gcc
mpirun -np $1 --rankfile ./rankfile_dbg -mca coll ^hcoll -x LD_PRELOAD=$LD_PRELOAD -x UCX_NET_DEVICES=mlx5_0:1 -x UCX_TLS=rc_x ./osu_mbw_mr_dummy2 -m 1:1 -W 192 -i 10000 # -i 1000000000

#--bind-to core 