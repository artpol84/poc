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
#include <mpi.h>

#define NBLOCKS 1024
#define BSIZE 2
#define SSIZE 4
#define SBUFSIZE (NBLOCKS * SSIZE)
#define RBUFSIZE (NBLOCKS * BSIZE)

int main(int argc, char **argv)
{
    int buf[SBUFSIZE];
    MPI_Datatype type;
    int rank;
    MPI_Request req;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Type_vector((NBLOCKS), BSIZE, SSIZE, MPI_INT, &type);
    MPI_Type_commit(&type);
    
    if( rank == 0 ){
        int i;

        MPI_Send(buf, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

        for(i=0; i < SBUFSIZE; i++){
            buf[i] = i+1;
        }
        MPI_Isend(buf, 1, type, 1, 0, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else {
        int i;
        MPI_Recv(buf, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        MPI_Recv(buf,RBUFSIZE, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Received: ");
        for(i=0; i<RBUFSIZE; i++){
            printf("%d ", buf[i]);
        }
        printf("\n");
    }

    MPI_Type_free(&type);
    
    MPI_Finalize();
}