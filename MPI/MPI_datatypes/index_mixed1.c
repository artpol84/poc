#include <stdio.h>
#include <mpi.h>
#include "utils.h"

#ifndef BASE_RANGE
#define BASE_RANGE 2
#endif

int main(int argc, char **argv)
{
    message_desc_t scenario[] = {
        { 3, 4, {2, 4, 1}, {4, 8, 3}},
        { 1, 1, {4}, {4} },
        { 2, 4, {2, 4}, {4, 8}},
        { 1, 1, {8}, {8} }
    };
    MPI_Init(&argc, &argv);
    test_mpi_index(NULL, BASE_RANGE, 0, 0, scenario, sizeof(scenario)/sizeof(scenario[0]));
    MPI_Finalize();
}
