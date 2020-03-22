#include <stdio.h>
#include <mpi.h>
#include "utils.h"

#ifndef BASE_IDX
#define BASE_IDX 2
#endif

int main(int argc, char **argv)
{
    message_desc_t scenario[] = {
        { 1, 4, {2}, {4} }
    };
    MPI_Init(&argc, &argv);
    test_mpi_index(NULL, 0, 0, BASE_IDX, scenario, sizeof(scenario)/sizeof(scenario[0]));
    MPI_Finalize();
}
