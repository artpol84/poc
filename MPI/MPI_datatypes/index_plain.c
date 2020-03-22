#include <stdio.h>
#include <mpi.h>
#include "utils.h"

int main(int argc, char **argv)
{
    message_desc_t scenario[] = {
        { 1, 1, {1}, {1} },
        { 1, 1, {2}, {2} },
        { 1, 1, {4}, {4} },
        { 1, 1, {6}, {6} }
    };
    MPI_Init(&argc, &argv);
    test_mpi_index(NULL, 0, 0, 0,
                   scenario, sizeof(scenario)/sizeof(scenario[0]), 1);
    MPI_Finalize();
}
