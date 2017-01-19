
CC = /home/artpol/WORK/OMPI/TASKS/2017_01_04_v2.x_exit_propagation/build/ompi-2.x/install/bin/mpicc

all: test_flock test_pthread

test_flock: test.c flock.c flock.h lock.h
	$(CC) -o test_flock -g -DMY_FLOCK=1 test.c flock.c

test_pthread: test.c pthread.c pthread.h lock.h
	$(CC) -o test_pthread -g -DMY_PTHREAD=1 test.c pthread.c -lpthread