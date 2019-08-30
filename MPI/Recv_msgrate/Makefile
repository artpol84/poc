
MPICC ?= mpicc

UTILS = osu_util.c osu_util_mpi.c
HEADERS = osu_util.h osu_util_mpi.h
BINARY = osu_mbw_mr

all: $(BINARY) $(BINARY)_dummy

$(BINARY): $(BINARY).c $(UTILS)
	$(MPICC) -o $(BINARY) -I. osu_mbw_mr.c $(UTILS)

$(BINARY)_dummy: $(BINARY)_dummy.c $(UTILS)
	$(MPICC) -o $(BINARY)_dummy -I. osu_mbw_mr_dummy.c $(UTILS)

clean:
	rm -f $(BINARY) $(BINARY)_dummy
