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

if [ -z "$SWIFT_HOSTS" ]; then
    echo "No SWIFT_HOSTS env found. You may need to check your swift_env.sh file if run is failing"
fi

if [ ! -f dat.conf ]; then
    echo "No local dat.conf is found. Consider creating one from /etc/dat.conf if your interface is not there"
fi

np=`get_nproc`
echo $np

nodes=`get_nnodes`
echo $nodes

threads=$(( $np / $ppn ))
if [ -n "$user_threads" ]; then
    threads=$user_threads
fi

OUT_FILE="impi_$OUT_FILE"

rm -f $OUT_FILE

echo "Running on $nodes nodes, with $ppn PPN, using $threads threads" |& tee -a $OUT_FILE

cmd="mpirun -np $(($nodes * $ppn)) -genv I_MPI_DAT_LIBRARY /usr/lib64/libdat2.so \
       -DAPL \
        -genv I_MPI_FABRICS shm:dapl \
        -genv I_MPI_DAPL_PROVIDER ofa-v2-mlx5_2-1u \
        -genv DAT_OVERRIDE ./dat.conf \
        -genv I_MPI_PIN on \
        -genv I_MPI_PERHOST $ppn \
        -genv I_MPI_DEBUG 4 \
        -hosts $SWIFT_HOSTS \
        ./swift_mpi \
                 --cosmology --hydro --self-gravity --stars --threads=$threads -n 2048 --cooling $infile" \

echo "$cmd" |& tee -a $OUT_FILE


set -x
ulimit -s unlimited

$cmd |& tee -a $OUT_FILE

rm -Rf restart 
