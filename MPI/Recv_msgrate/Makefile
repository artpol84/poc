
MPICC ?= mpicc
UTILS = osu_util.c osu_util_mpi.c
HEADERS = osu_util.h osu_util_mpi.h
BINARY = osu_mbw_mr

$(BINARY): $(BINARY).c $(UTILS)
	$(MPICC) -o $(BINARY) -I. osu_mbw_mr.c $(UTILS)

clean:
	rm -f $(BINARY)
