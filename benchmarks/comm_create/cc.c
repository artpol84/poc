#include <stdio.h>
#include <mpi.h>

#include "pattern.h"

#define groups_cnt (sizeof(group_ranks)/sizeof(group_ranks[0]))
MPI_Comm comm[groups_cnt];
MPI_Group g_world, groups[groups_cnt];

#include <time.h>
#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
})

int main(int argc, char **argv)
{
    int i = 1, size, rank;
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    double create_time[3];

    MPI_Comm_group(MPI_COMM_WORLD, &g_world);

    double ts = GET_TS();
    for(i = 0; i < groups_cnt; i++){
         MPI_Group_incl(g_world, 3, group_ranks[i], &groups[i]);
         MPI_Comm_create(MPI_COMM_WORLD, groups[i], &comm[i]);
         MPI_Barrier(MPI_COMM_WORLD);
    }
    ts = GET_TS() - ts;
    MPI_Reduce(&ts, &create_time[0], 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ts, &create_time[1], 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Reduce(&ts, &create_time[2], 1, MPI_DOUBLE, MPI_MIN, 0, MPI_COMM_WORLD);
    if( 0 == rank ){
        printf("Comm create time: %lf / %lf / %lf\n", create_time[0]/size, create_time[1], create_time[2]);
    }

/*
    for(i=0; i<groups_cnt; i++){
        MPI_Comm_free(&comm[i]);
    }
    printf("free time: %lf\n", GET_TS() - ts);
*/
    
    MPI_Finalize();
}