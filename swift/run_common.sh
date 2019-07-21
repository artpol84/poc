#!/bin/bash

get_hosts()
{
    local HOSTS=""
    if [ -n "$SLURM_JOB_NODELIST" ]; then
	HOSTS=`scontrol show hostnames $SLURM_JOB_NODELIST | tr '\n' ','`
    elif [ -n "$SWIFT_HOSTS" ]; then
	HOSTS=$SWIFT_HOSTS
    else
	echo "Cannot resolve hostnames"
	exit 1
    fi
    echo -n "$HOSTS"
}

get_nproc()
{
    local HOSTS=`get_hosts`
    hname=`echo $HOSTS | tr ',' '\n' | head -n 1`
    np=`ssh $hname nproc`
    echo $np
}

get_nnodes()
{
    local HOSTS=`get_hosts`
    hcnt=`echo $HOSTS | tr ',' '\n' | awk 'BEGIN{ total=0; } { if($1 != "") total += 1 } END{ print total }'`
    echo "$hcnt"
}

infile=$1
if [ -z "$infile" ]; then
    echo "Need the input file!"
    exit 1
fi
tmp=${infile%\.yml};
out_sfx=E${tmp##eagle_}
shift

ppn=$1
if [ -z "$ppn" ]; then
    echo "Need PPN input parameter!"
    exit 1
fi
shift 

user_threads=""
if [ -n "$1" ]; then
    user_threads=$1
    shift
fi

OUT_FILE=${MPI}_${CC}_swift_${out_sfx}_${nodes}Nx${ppn}PPNx${threads}THR_`date -Iminutes`.log