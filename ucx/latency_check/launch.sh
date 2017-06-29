#!/bin/bash

#UCX_BASE=/hpc/local/benchmarks/hpcx_install_Tuesday/hpcx-gcc-redhat7.2/ucx/
UCX_BASE=/labhome/artemp/mtr_scrap/SLURM/2017_05_03_ucx/ucx/0debug/install/
export LD_LIBRARY_PATH="$UCX_BASE/lib:$LD_LIBRARY_PATH"

export UCX_NET_DEVICES=mlx5_0:1
export UCX_TLS=ud
export UCX_MEM_MALLOC_HOOKS=no

$@