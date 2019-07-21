#!/bin/bash

SCRIPT_PATH=$0
if [ -L "$SCRIPT_PATH" ]; then
    SCRIPT_PATH=`readlink $SCRIPT_PATH`
fi
SCRIPT_DIR=`dirname $SCRIPT_PATH`

if [ -f "swift_env.sh" ]; then
    # If user defined a local env - use it
    . swift_env.sh
else
    # Use the default env file
    . $SCRIPT_DIR/swift_env.sh
fi

. $SCRIPT_DIR/run_common.sh

if [ -z "$SWIFT_INST_DIR" ]; then
    echo "No SWIFT_INST_DIR env found. You may need to check your swift_env.sh file if run is failing"
else
    export LD_LIBRARY_PATH="$SWIFT_INST_DIR/build/lib/:$LD_LIBRARY_PATH"
fi

np=`get_nproc`
echo "NP=$np"

nodes=`get_nnodes`
echo "NODEs=$nodes"

echo "ppn=$ppn"

threads=$(( $np / $ppn ))
if [ -n "$user_threads" ]; then
    threads=$user_threads
fi

rm -f $OUT_FILE

echo "Running on $nodes nodes, with $ppn PPN, using $threads threads" |& tee -a $OUT_FILE

set -x
ulimit -s unlimited
cmd="mpirun -np $(($nodes * $ppn)) --map-by node:PE=$threads -x UCX_NET_DEVICES=mlx5_2:1 -x UCX_TLS=rc_x -x UCX_USE_MT_MUTEX=y -x UCX_RNDV_THRESH=1G -x UCX_ZCOPY_THRESH=1G \
      ./swift_mpi --cosmology --hydro --self-gravity --stars --threads=$threads -n 2048 --cooling $infile"
      
echo $cmd | tee -a $OUT_FILE

$cmd |& tee -a $OUT_FILE

rm -fR ./restart
