#include <stdio.h>
#include <mpi.h>
#include "utils.h"

#ifndef BASE_RANGE
#define BASE_RANGE 1
#endif


int main(int argc, char **argv)
{
    message_desc_t scenario[] = {
        { 3, 4, {2, 4, 8}, {4, 8, 16} },
        { 3, 4, {2, 2, 2}, {4, 4, 4} }
    };

    MPI_Init(&argc, &argv);
    test_mpi_index(NULL, BASE_RANGE, 0, 0,
                   scenario, sizeof(scenario)/sizeof(scenario[0]), 1);
    MPI_Finalize();
}
