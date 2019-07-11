#!/bin/bash

get_nproc()
{
    local HOSTS=$1
    hname=`echo $HOSTS | tr ',' '\n' | head -n 1`
    np=`ssh $hname nproc`
    echo $np
}

get_nnodes()
{
    local HOSTS=$1
    hcnt=`echo $HOSTS | tr ',' '\n' | wc -l`
    echo "$hcnt"
}


ppn=$1
if [ -z "$ppn" ]; then
    echo "Need PPN input parameter!"
    exit 1
fi
shift 

debug=$1
if [ "$debug" = "debug" ]; then
    echo "DEBUG MODE"
    shift
fi

module load intel/ics-18.0.4

export LD_LIBRARY_PATH="/labhome/artemp/SWIFT/2019_07_09_Profiling_IMPI/build/lib/:$LD_LIBRARY_PATH"

HOSTS=clx-hercules-002,clx-hercules-009,clx-hercules-010,clx-hercules-014

np=`get_nproc $HOSTS`
echo $np

nodes=`get_nnodes $HOSTS`
echo $nodes

echo "ppn=$ppn"

threads=$(( $np / $ppn ))
echo "Running on $nodes nodes, with $ppn PPN, using $threads threads"

if [ $debug ]; then
    mpirun -np $(($nodes * $ppn)) -genv I_MPI_DAT_LIBRARY /usr/lib64/libdat2.so \
           -DAPL \
            -genv I_MPI_FABRICS shm:dapl \
            -genv I_MPI_DAPL_PROVIDER ofa-v2-mlx5_2-1u \
            -genv DAT_OVERRIDE ./dat.conf \
            -genv I_MPI_PIN on \
            -genv I_MPI_PIN on \
            -genv I_MPI_PERHOST $ppn \
            -genv I_MPI_DEBUG 4 \
            -hosts $HOSTS \
            ./hello_c
else
    set -x
    ulimit -s unlimited
    mpirun -np $(($nodes * $ppn)) -genv I_MPI_DAT_LIBRARY /usr/lib64/libdat2.so \
           -DAPL \
            -genv I_MPI_FABRICS shm:dapl \
            -genv I_MPI_DAPL_PROVIDER ofa-v2-mlx5_2-1u \
            -genv DAT_OVERRIDE ./dat.conf \
            -genv I_MPI_PIN on \
            -genv I_MPI_PIN on \
            -genv I_MPI_PERHOST $ppn \
            -genv I_MPI_DEBUG 4 \
            -hosts $HOSTS \
            ../../swift_mpi \
                     --cosmology --hydro --self-gravity --stars --threads=$threads -n 64 --cooling eagle_25.yml 2>&1 | tee swift_${nodes}x${ppn}x${threads}.log
fi