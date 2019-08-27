test=mpi_window_test

all: ${test}.c
	mpicc -o ${test} -g -O0 -fopenmp ${test}.c

clean:
	rm -f ${test}
