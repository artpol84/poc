test=mpi_window_test

ompi: ${test}.c
	mpicc -o ${test} -g -O0 -fopenmp ${test}.c

mpich: ${test}.c
	mpicc -o ${test} -g -O0 -fopenmp ${test}.c

impi: ${test}.c
	mpiicc -o ${test} -g -O0 -qopenmp ${test}.c

clean:
	rm -f ${test}
