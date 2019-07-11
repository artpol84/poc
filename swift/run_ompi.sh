#!/bin/bash

get_nproc()
{
    local HOSTS=$1
    hname=`scontrol show hostnames $SLURM_JOB_NODELIST | tr ',' '\n' | head -n 1`
    np=`ssh $hname nproc`
    echo $np
}

get_nnodes()
{
    local HOSTS=$1
    hcnt=`scontrol show hostnames $SLURM_JOB_NODELIST | tr ',' '\n' | wc -l`
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

module load hpcx-gcc-mt

export LD_LIBRARY_PATH="/labhome/artemp/SWIFT/2019_07_09_Profiling/build/lib/:$LD_LIBRARY_PATH"

np=`get_nproc $HOSTS`
echo $np

nodes=`get_nnodes $HOSTS`
echo $nodes

echo "ppn=$ppn"

threads=$(( $np / $ppn ))
echo "Running on $nodes nodes, with $ppn PPN, using $threads threads"

set -x
ulimit -s unlimited
mpirun -np $(($nodes * $ppn)) --map-by node:PE=$threads \
     ../../swift_mpi --cosmology --hydro --self-gravity --stars --threads=$threads -n 1024 --cooling eagle_25.yml 2>&1 | tee output.log

