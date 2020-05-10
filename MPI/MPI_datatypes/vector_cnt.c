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
#include <string.h>
#include <stdlib.h>
#include <mpi.h>

#define NCOUNT  4
#define NBLOCKS 2
#define BSIZE 2
#define SSIZE 4
#define SBUFSIZE (NBLOCKS * SSIZE * NCOUNT)
#define RBUFSIZE (NBLOCKS * BSIZE * NCOUNT)

int main(int argc, char **argv)
{
    int buf[SBUFSIZE];
    MPI_Datatype vtype, type;
    int rank;
    MPI_Request req;
    
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    MPI_Type_vector((NBLOCKS), BSIZE, SSIZE, MPI_INT, &vtype);
    MPI_Type_commit(&vtype);

    // Resize to ensure regular striging with multi-count sends
    MPI_Type_create_resized(vtype, 0, sizeof(int) * SSIZE * NBLOCKS, &type);
    MPI_Type_commit(&type);
    
    if( rank == 0 ){
        int i;

        MPI_Send(buf, 1, MPI_INT, 1, 0, MPI_COMM_WORLD);

        for(i=0; i < SBUFSIZE; i++){
            buf[i] = i+1;
        }

        MPI_Isend(buf, NCOUNT, type, 1, 0, MPI_COMM_WORLD, &req);
        MPI_Wait(&req, MPI_STATUS_IGNORE);
    } else {
        int i, j;

        MPI_Recv(buf, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        memset(buf, 0, sizeof(buf));
        MPI_Recv(buf,NCOUNT, type, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        printf("Received: ");
        for(i=0; i<SBUFSIZE; i++){
            printf("%d ", buf[i]);
        }
        // Verify
        for(i=0; i < NCOUNT; i++){
            for(j = 0; j < NBLOCKS; j++) {
                int start = (SSIZE * NBLOCKS) * i + SSIZE * j;
                int k;
                // First verify the block data
                for(k = 0; k < BSIZE; k++){
                    if( buf[start + k] != (start + k + 1) ) {
                        printf("Mismatch in block area: expect %d, got %d\n",
                                (start + k + 1), buf[start + k]);
                        exit(1);
                    }
                }
                // Next, verify that remaining is 0s
                for(; k < SSIZE; k++){
                    if( buf[start + k] != 0 ) {
                        printf("Mismatch in stride area: expect %d, got %d\n",
                                0, buf[start + k]);
                        exit(1);
                    }
                }
            }
        }
        printf("\n");
    }

    MPI_Type_free(&type);
    MPI_Type_free(&vtype);
    
    MPI_Finalize();
}