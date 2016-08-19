
CFLAGS ?= -g -O2
MPICC ?= mpicc

SOURCES = options.c threads.c mr_th_nb.c 
HEADERS = options.h threads.h

all: $(SOURCES) $(HEADERS)
	$(MPICC) $(CFLAGS) -o mr_th_nb $(SOURCES)

clean:
	rm -f mr_th_nb