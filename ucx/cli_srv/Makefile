
CC = /labhome/artemp/mtr_scrap/UCX/for_hackathon/openmpi-2.0.1/install/bin/mpicc
CFLAGS = -g -O0 -I/labhome/artemp/mtr_scrap/UCX/for_hackathon/ucx/install/include/ 
LDFLAGS = -L/labhome/artemp/mtr_scrap/UCX/for_hackathon/ucx/install/lib -lucp -lucs

all: ucx_clisrv ucp_hello_world

ucx_clisrv: ucx_clisrv.c
	$(CC) -o ucx_clisrv $(CFLAGS) $(LDFLAGS) ucx_clisrv.c
	
ucp_hello_world: ucp_hello_world.c
	$(CC) -o ucp_hello_world $(CFLAGS) $(LDFLAGS) ucp_hello_world.c
	