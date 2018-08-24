#include <stdio.h>

void sync_shmem_srv_init();
void sync_shmem_cli_init();
void sync_shmem_barrier();
void sync_shmem_allgather(double in, double **out);
int sync_shmem_nranks();
int sync_shmem_rank();
