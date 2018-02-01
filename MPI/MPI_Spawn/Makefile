BASE_P=<mpi-path>/bin

all: clean
	$(BASE_P)/mpicc -o slave slave.c
	$(BASE_P)/mpicc -o master master.c

clean:
	rm -f master slave
