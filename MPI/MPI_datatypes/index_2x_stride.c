#include <stdio.h>
#include <mpi.h>
#include "utils.h"

int main(int argc, char **argv)
{
    message_desc_t scenario[] = {
        { 1, 4, {2}, {4} },
        { 1, 4, {2}, {4} }
    };
    MPI_Init(&argc, &argv);
    test_mpi_index(NULL, 0, 0, 0, scenario, sizeof(scenario)/sizeof(scenario[0]));
    MPI_Finalize();
}
