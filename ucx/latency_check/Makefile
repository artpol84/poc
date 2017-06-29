
#UCX_BASE = /hpc/local/benchmarks/hpcx_install_Tuesday/hpcx-gcc-redhat7.2/ucx/
UCX_BASE = /labhome/artemp/mtr_scrap/SLURM/2017_05_03_ucx/ucx/0debug/install/
CC := gcc
CFLAGS = -g -O0 -I$(UCX_BASE)/include
LDFLAGS = -L$(UCX_BASE)/lib -lucp -lucs


all: ucp_latency 

ucp_latency: ucp_latency.c
	$(CC) -o ucp_latency $(CFLAGS) $(LDFLAGS) ucp_latency.c
