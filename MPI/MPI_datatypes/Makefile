MPICC ?= mpicc

UTILS_SRC = utils.c
UTILS = $(UTILS_SRC) utils.h

TESTS = index_ilv_w_block.c index_mixed1.c index_plain.c index_regular.c index_regular_ilv.c vector.c index_2x_stride.c index_2x_ilv.c

all: $(UTILS) $(TESTS) 
	$(MPICC) -o index_ilv_w_block -g index_ilv_w_block.c $(UTILS) 
	$(MPICC) -o index_mixed1 -g index_mixed1.c $(UTILS)
	$(MPICC) -o index_plain -g index_plain.c $(UTILS)
	$(MPICC) -o index_regular_s0 -DSTART_IDX=0 -g index_regular.c $(UTILS)
	$(MPICC) -o index_regular_s2 -g index_regular.c $(UTILS)
	$(MPICC) -o index_regular_ilv -g index_regular_ilv.c $(UTILS) 
	$(MPICC) -o index_2x_stride -g index_2x_stride.c $(UTILS) 
	$(MPICC) -o index_2x_ilv -g index_2x_ilv.c $(UTILS) 
	$(MPICC) -o vector -g vector.c

clean:
	rm -f index_ilv_w_block index_mixed1 index_plain index_regular_s* index_regular_ilv vector index_2x_stride index_2x_ilv
