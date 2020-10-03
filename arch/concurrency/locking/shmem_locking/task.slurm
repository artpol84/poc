#!/bin/bash

#SBATCH --job-name=ShmemLockTest
#SBATCH --nodes=1 --ntasks-per-node=8

cd $SLURM_SUBMIT_DIR

./run.sh 
