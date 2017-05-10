#!/bin/bash

UCX_BASE=<UCX install path>

export LD_LIBRARY_PATH="$UCX_BASE/lib:$LD_LIBRARY_PATH"

export UCX_NET_DEVICES=mlx5_0:1
export UCX_TLS=rc

$@