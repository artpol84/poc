MPIPATH = /Users/artemp/Dev/SW/ompi/install/
MPICC = $(MPIPATH)/bin/mpicc
MPIINC = $(MPIPATH)/include
LDFLAGS = 

all: dbtree.c
	$(MPICC) -o dbtree -O0 -g -I$(MPIINC) dbtree.c $(LDFLAGS)