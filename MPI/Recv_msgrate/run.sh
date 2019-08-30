#!/bin/bash


module load hpcx-gcc


mpirun -np $1 --rankfile ./rankfile --bind-to core \
    ./osu_mbw_mr -m 1:1 -i 10000