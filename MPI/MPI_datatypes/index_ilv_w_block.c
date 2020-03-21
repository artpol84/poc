#include <stdio.h>
#include <mpi.h>
#include "utils.h"

int main(int argc, char **argv)
{
    message_desc_t scenario[] = {
        { 3, 4, {2, 2, 2}, {4, 4, 4} },
        { 1, 1, {2}, {2} }
    };
    MPI_Init(&argc, &argv);
    test_mpi_index(NULL, 0, 0, 0, scenario, sizeof(scenario)/sizeof(scenario[0]));
    MPI_Finalize();
}
