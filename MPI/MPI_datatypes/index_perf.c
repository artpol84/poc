#include <stdio.h>
#include <mpi.h>
#include "utils.h"


int main(int argc, char **argv)
{
    message_desc_t scenario[] = {
        { 1, 1, { 30 }, {256} },
        { 1, 23, {30*8}, {60*8} },

    };
    MPI_Init(&argc, &argv);
    test_mpi_index(NULL, 0, 0, 0, scenario, sizeof(scenario)/sizeof(scenario[0]));
    MPI_Finalize();
}
