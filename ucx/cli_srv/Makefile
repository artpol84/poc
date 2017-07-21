UCX_PATH=
MPI_PATH=

CC = $(MPI_PATH)/bin/mpicc
CFLAGS = -g -O0 -I$(UCX_PATH)/include/ 
LDFLAGS = -L$(UCX_PATH)/lib -lucp -lucs

all: ucx_clisrv

ucx_clisrv: ucx_clisrv.c
	$(CC) -o ucx_clisrv $(CFLAGS) $(LDFLAGS) ucx_clisrv.c
