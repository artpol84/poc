#!/bin/bash

UCX_BASE=/hpc/local/benchmarks/hpcx_install_Tuesday/hpcx-gcc-redhat7.2/ucx/

export LD_LIBRARY_PATH="$UCX_BASE/lib:$LD_LIBRARY_PATH"

export UCX_NET_DEVICES=mlx5_0:1
export UCX_TLS=rc
#export UCX_ZCOPY_THRESH=inf

$@