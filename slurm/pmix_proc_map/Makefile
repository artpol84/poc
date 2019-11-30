SLURM_SRC=/home/user/slurm/slurm-master
SLURM_LIB=/sandbox/slurm/lib

all:
	gcc -o main -I${SLURM_SRC}/src/common -L${SLURM_LIB} -O2 main.c -lslurm -Wl,-rpath=${SLURM_LIB}
