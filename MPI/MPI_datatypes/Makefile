MPICC ?= /home/artpol/sandbox/openmpi-3.1.3/bin/mpicc

UTILS_SRC = utils.c
UTILS = $(UTILS_SRC) utils.h

TESTS = index_ilv_w_block.c index_mixed1.c index_plain.c index_regular_str.c index_regular_ilv.c vector.c index_2x_str.c index_2x_ilv.c

all: $(UTILS) $(TESTS) 
	$(MPICC) -o index_ilv_w_block_offs0 -DBASE_RANGE=0 -g index_ilv_w_block.c $(UTILS) 
	$(MPICC) -o index_ilv_w_block_offsM -DBASE_RANGE=1 -g index_ilv_w_block.c $(UTILS) 
	$(MPICC) -o index_mixed1_offs0 -DBASE_RANGE=0 -g index_mixed1.c $(UTILS)
	$(MPICC) -o index_mixed1_offsM -DBASE_RANGE=2 -g index_mixed1.c $(UTILS)
	$(MPICC) -o index_plain_offs0 -DBASE_RANGE=0 -g index_plain.c $(UTILS)
	$(MPICC) -o index_plain_offsM -DBASE_RANGE=1 -g index_plain.c $(UTILS)
	$(MPICC) -o index_regular_str_offs0 -DBASE_IDX=0 -g index_regular_str.c $(UTILS)
	$(MPICC) -o index_regular_str_offsM -DBASE_IDX=2 -g index_regular_str.c $(UTILS)
	$(MPICC) -o index_regular_ilv_offs0 -DBASE_IDX=0 -g index_regular_ilv.c $(UTILS) 
	$(MPICC) -o index_regular_ilv_offsM -DBASE_IDX=3 -g index_regular_ilv.c $(UTILS) 
	$(MPICC) -o index_2x_str_offs0 -DBASE_RANGE=0 -g index_2x_str.c $(UTILS) 
	$(MPICC) -o index_2x_str_offsM -DBASE_RANGE=1 -g index_2x_str.c $(UTILS) 
	$(MPICC) -o index_2x_ilv_offs0 -DBASE_RANGE=0 -g index_2x_ilv.c $(UTILS) 
	$(MPICC) -o index_2x_ilv_offsM -DBASE_RANGE=1 -g index_2x_ilv.c $(UTILS) 
	$(MPICC) -o vector -g vector.c
	$(MPICC) -o index_perf -g index_perf.c $(UTILS) 

clean:
	rm -f index_ilv_w_block_offs* index_mixed1_offs* index_plain_offs* index_regular_str_offs* index_regular_ilv_offs* vector index_2x_str_offs* index_2x_ilv_offs*
