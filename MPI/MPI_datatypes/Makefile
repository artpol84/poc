MPICC ?= mpicc

UTILS_SRC = utils.c
UTILS = $(UTILS_SRC) utils.h

TESTS = index_ilv_w_block.c index_mixed1.c index_plain.c index_regular.c index_regular_ilv.c vector.c

all: $(UTILS) $(TESTS) 
	$(MPICC) -o index_ilv_w_block -g index_ilv_w_block.c 
	$(MPICC) -o index_mixed1 -g index_mixed1.c $(UTILS)
	$(MPICC) -o index_plain -g index_plain.c
	$(MPICC) -o index_regular -g index_regular.c
	$(MPICC) -o index_regular_ilv -g index_regular_ilv.c 
	$(MPICC) -o vector -g vector.c

clean:
	rm -f index_ilv_w_block index_mixed1 index_plain index_regular index_regular_ilv vector
