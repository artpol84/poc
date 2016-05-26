all: slurm slurm_my

slurm: hostlist.c hostlist.h output.c output.h slurm.c
	gcc -g -o slurm hostlist.c output.c slurm.c

slurm_my: hostlist.c hostlist.h output.c output.h slurm_my.c
	gcc -g -o slurm_my hostlist.c output.c slurm_my.c

clean: 
	rm -f slurm slurm_my *.jpg *.dot