#!/bin/bash

export LD_LIBRARY_PATH="/labhome/artemp/mtr_scrap/UCX/for_hackathon/ucx/install/lib:$LD_LIBRARY_PATH"

export UCX_NET_DEVICES=mlx5_0:1
export UCX_TLS=ud

/labhome/artemp/mtr_scrap/UCX/for_hackathon/openmpi-2.0.1/install/bin/mpirun \
        -np $1 --mca btl self,vader,tcp \
        --mca pml ob1 --mca mtl ^mxm  \
        --map-by node \
        ./ucx_clisrv
