
CC = mpicc
CFLAGS = -g -O0 -I/labhome/artemp/mtr_scrap/UCX/for_hackathon/ucx/install/include/ 
LDFLAGS = -L/labhome/artemp/mtr_scrap/UCX/for_hackathon/ucx/install/lib -lucp

ucx_clisrv: ucx_clisrv.c
	$(CC) -o ucx_clisrv $(CFLAGS) $(LDFLAGS) ucx_clisrv.c