/*
Copyright 2020 Artem Y. Polyakov <artpol84@gmail.com>
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may
be used to endorse or promote products derived from this software without specific
prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <mpi.h>

int main(int argc, char **argv)
{
    char *buf = NULL, sync[1], *base_addr = NULL;
    MPI_Datatype type;
    int rank;
    MPI_Request req;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int displs[] = { 144480, 169200 };
    int lengths[] = { 5760, 5760 };
    int msize = displs[1] - displs[0] + lengths[1];
    buf = calloc(msize, 1);
    base_addr = buf - 0x23460;
    int i, j;


    MPI_Type_indexed(2, lengths, displs, MPI_CHAR, &type);
    MPI_Type_commit(&type);

    
    if( rank == 0 ){
        int i;
        /* Ensure wireup */
        MPI_Send(sync, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

        /* Init */
        int start = 'A';
        for(i = 0; i < 2; i++){
            char *ptr = buf + displs[i] - displs[0];
            for(j = 0; j < lengths[i]; j++){
                ptr[j] = 'A' + (start++) % ('Z' - 'A' + 1);
            }
        }

        /* Initiate first UMR creation */
        MPI_Isend(base_addr, 1, type, 1, 0, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
        printf("TEST successful\n");
    } else {
        int i;
        MPI_Recv(sync, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        
        {
            int delay = 0;
            while(delay) {
                sleep(1);
            }
        }
        
        MPI_Irecv(base_addr, 1, type, 0, 0, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);

        int start = 'A';
        for(i = 0; i < 2; i++){
            char *ptr = buf + displs[i] - displs[0];
            for(j = 0; j < lengths[i]; j++){
                if( ptr[j] != 'A' + (start++) % ('Z' - 'A' + 1) ) {
                    printf("ERROR!!!\n");
                }
            }
        }

    }

    MPI_Type_free(&type);
    
    MPI_Finalize();

    return 0;
}
