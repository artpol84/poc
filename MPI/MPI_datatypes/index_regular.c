#include <stdio.h>
#include <mpi.h>
#include "utils.h"

#ifndef START_IDX
#define START_IDX 2
#endif

int main(int argc, char **argv)
{
    message_desc_t scenario[] = {
        { 1, 4, {2}, {4} }
    };
    MPI_Init(&argc, &argv);
    test_mpi_index(NULL, 0, 0, START_IDX, scenario, sizeof(scenario)/sizeof(scenario[0]));
    MPI_Finalize();
}
