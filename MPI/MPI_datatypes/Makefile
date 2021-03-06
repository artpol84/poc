# Copyright 2020 Artem Y. Polyakov <artpol84@gmail.com>
# Redistribution and use in source and binary forms, with or without modification,
# are permitted provided that the following conditions are met:
# 1. Redistributions of source code must retain the above copyright notice, this
# list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation and/or
# other materials provided with the distribution.
# 3. Neither the name of the copyright holder nor the names of its contributors may
# be used to endorse or promote products derived from this software without specific
# prior written permission.
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
# ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
# IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
# INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
# BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
# DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
# LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
# OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
# OF THE POSSIBILITY OF SUCH DAMAGE.


MPICC ?= mpicc
#MPICC ?= /home/artpol/sandbox/openmpi-3.1.3/bin/mpicc

UTILS_SRC = utils.c
UTILS = $(UTILS_SRC) utils.h

TESTS = index_ilv_w_block.c index_mixed1.c index_plain.c index_regular_str.c index_regular_ilv.c vector.c vector_cnt.c vector_cnt2.c vector_rcache.c vector_2d.c  index_2x_str.c index_2x_ilv.c index_repro_bug1.c res_usage.c dt_create_overhead.c

all: $(UTILS) $(TESTS) 
	$(MPICC) -o index_ilv_w_block_offs0 -DBASE_RANGE=0 -g index_ilv_w_block.c $(UTILS) 
	$(MPICC) -o index_ilv_w_block_offsM -DBASE_RANGE=1 -g index_ilv_w_block.c $(UTILS) 
	$(MPICC) -o index_ilv_w_block_rcvdt_exp   -DRECV_TYPE=1 -g index_ilv_w_block.c $(UTILS) 
	$(MPICC) -o index_ilv_w_block_rcvdt_unexp -DRECV_TYPE=1 -DFORCE_UNEXP=1 -g index_ilv_w_block.c $(UTILS) 
	$(MPICC) -o index_mixed1_offs0 -DBASE_RANGE=0 -g index_mixed1.c $(UTILS)
	$(MPICC) -o index_mixed1_offsM -DBASE_RANGE=2 -g index_mixed1.c $(UTILS)
	$(MPICC) -o index_mixed1_rcvdt_exp   -DRECV_TYPE=1 -g index_mixed1.c $(UTILS)
	$(MPICC) -o index_mixed1_rcvdt_unexp -DRECV_TYPE=1 -DFORCE_UNEXP=1  -g index_mixed1.c $(UTILS)
	$(MPICC) -o index_plain_offs0 -DBASE_RANGE=0 -g index_plain.c $(UTILS)
	$(MPICC) -o index_plain_offsM -DBASE_RANGE=1 -g index_plain.c $(UTILS)
	$(MPICC) -o index_repro_bug1 -g index_repro_bug1.c $(UTILS)
	$(MPICC) -o index_plain_rcvdt_exp   -DRECV_TYPE=1 -g index_plain.c $(UTILS)
	$(MPICC) -o index_plain_rcvdt_unexp -DRECV_TYPE=1 -DFORCE_UNEXP=1 -g index_plain.c $(UTILS)
	$(MPICC) -o index_regular_str_offs0 -DBASE_IDX=0 -g index_regular_str.c $(UTILS)
	$(MPICC) -o index_regular_str_offsM -DBASE_IDX=2 -g index_regular_str.c $(UTILS)
	$(MPICC) -o index_regular_str_rcvdt_exp   -DRECV_TYPE=1 -g index_regular_str.c $(UTILS)
	$(MPICC) -o index_regular_str_rcvdt_unexp -DRECV_TYPE=1 -DFORCE_UNEXP=1 -g index_regular_str.c $(UTILS)
	$(MPICC) -o index_regular_ilv_offs0 -DBASE_IDX=0 -g index_regular_ilv.c $(UTILS) 
	$(MPICC) -o index_regular_ilv_offsM -DBASE_IDX=3 -g index_regular_ilv.c $(UTILS) 
	$(MPICC) -o index_regular_ilv_rcvdt_exp   -DRECV_TYPE=1 -g index_regular_ilv.c $(UTILS) 
	$(MPICC) -o index_regular_ilv_rcvdt_unexp -DRECV_TYPE=1 -DFORCE_UNEXP=1 -g index_regular_ilv.c $(UTILS) 
	$(MPICC) -o index_2x_str_offs0 -DBASE_RANGE=0 -g index_2x_str.c $(UTILS) 
	$(MPICC) -o index_2x_str_offsM -DBASE_RANGE=1 -g index_2x_str.c $(UTILS) 
	$(MPICC) -o index_2x_str_rcvdt_exp   -DRECV_TYPE=1 -g index_2x_str.c $(UTILS) 
	$(MPICC) -o index_2x_str_rcvdt_unexp -DRECV_TYPE=1 -DFORCE_UNEXP=1 -g index_2x_str.c $(UTILS) 
	$(MPICC) -o index_2x_ilv_offs0 -DBASE_RANGE=0 -g index_2x_ilv.c $(UTILS) 
	$(MPICC) -o index_2x_ilv_offsM -DBASE_RANGE=1 -g index_2x_ilv.c $(UTILS) 
	$(MPICC) -o index_2x_ilv_rcvdt_exp   -DRECV_TYPE=1 -g index_2x_ilv.c $(UTILS) 
	$(MPICC) -o index_2x_ilv_rcvdt_unexp -DRECV_TYPE=1 -DFORCE_UNEXP=1 -g index_2x_ilv.c $(UTILS) 
	$(MPICC) -o dt_create_overhead -g dt_create_overhead.c $(UTILS)
	$(MPICC) -o vector -g vector.c
	$(MPICC) -o vector_cnt -g vector_cnt.c
	$(MPICC) -o vector_cnt2 -g vector_cnt2.c
	$(MPICC) -o vector_rcache -g vector_rcache.c
	$(MPICC) -o vector_2d -g vector_2d.c
	$(MPICC) -o index_perf -g index_perf.c $(UTILS) 
	$(MPICC) -o index_two_dts -g index_two_dts.c $(UTILS) 
	$(MPICC) -o index_zcopy_multi_offs0 -g index_zcopy_multi.c $(UTILS) 
	$(MPICC) -o index_zcopy_multi_rcvdt_exp   -DRECV_TYPE=1 -g index_zcopy_multi.c $(UTILS) 
	$(MPICC) -o index_zcopy_multi_rcvdt_unexp -DRECV_TYPE=1 -DFORCE_UNEXP=1 -g index_zcopy_multi.c $(UTILS) 
	$(MPICC) -o res_usage -g res_usage.c $(UTILS)


clean:
	rm -f *_offs* *_rcvdt_* vector vector_2d vector_cnt vector_cnt2 vector_rcache index_perf index_two_dts index_zcopy_multi index_repro_bug1 res_usage dt_create_overhead
