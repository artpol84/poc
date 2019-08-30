
MPICC ?= mpicc

UTILS = osu_util.c osu_util_mpi.c
HEADERS = osu_util.h osu_util_mpi.h
BINARY = osu_mbw_mr

all: $(BINARY) $(BINARY)_dummy $(BINARY)_dummy2

$(BINARY): $(BINARY).c $(UTILS)
	$(MPICC) -o $(BINARY) -I. -O3 -g osu_mbw_mr.c $(UTILS)

$(BINARY)_dummy: $(BINARY)_dummy.c $(UTILS)
	$(MPICC) -o $(BINARY)_dummy -g -O3 -I. osu_mbw_mr_dummy.c $(UTILS)

$(BINARY)_dummy2: $(BINARY)_dummy2.c $(UTILS)
	gcc -c -Wall -Werror -fpic osu_mbw_mr_dummy2_lib.c
	gcc -shared -o libdummy2.so osu_mbw_mr_dummy2_lib.o
	$(MPICC) -o $(BINARY)_dummy2 -g -O3 -I. osu_mbw_mr_dummy2.c $(UTILS) -L. -ldummy2

clean:
	rm -f $(BINARY) $(BINARY)_dummy $(BINARY)_dummy2 libdummy2.so
