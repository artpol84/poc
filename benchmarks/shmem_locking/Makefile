
CC = /home/artpol/WORK/PMIx/TASKS/2017_02_26_dstore_atomics/ompi-build/openmpi-1.6.5/install/bin/mpicc
CFLAGS = -O3 -g
LDFLAGS = -lrt

TARGETS = test_flock test_pthread test_pthread_prio test_pthread_mutex test_dummy

all: $(TARGETS)

test_flock: test.c flock.c flock.h lock.h Makefile
	$(CC) -o test_flock $(CFLAGS) $(LDFLAGS) -DMY_FLOCK=1 test.c flock.c

test_dummy: test.c dummy.c dummy.h lock.h Makefile
	$(CC) -o test_dummy $(CFLAGS) $(LDFLAGS) -DMY_DUMMY=1 test.c dummy.c

test_pthread: test.c pthread.c pthread.h lock.h Makefile
	$(CC) -o test_pthread $(CFLAGS) $(LDFLAGS) -DMY_PTHREAD=1 test.c pthread.c -lpthread

test_pthread_prio: test.c pthread.c pthread.h lock.h Makefile
	$(CC) -o test_pthread_prio $(CFLAGS) $(LDFLAGS) -DMY_PTHREAD=1 -DMY_PTHREAD_PRIO=1 test.c pthread.c -lpthread
	
test_pthread_mutex: test.c pthread_mutex.c pthread_mutex.h lock.h Makefile
	$(CC) -o test_pthread_mutex $(CFLAGS) $(LDFLAGS) -DMY_PTHREAD_MUTEX=1 test.c pthread_mutex.c -lpthread

test_c11_atomics: c11_atomics.c c11_atomics.h test.c lock.h Makefile
	$(CC) -o test_c11_atomics $(CFLAGS) $(LDFLAGS) -DMY_C11_ATOMICS=1 test.c c11_atomics.c -lpthread

test_c11_atomics_spin: c11_atomics_spin.c c11_atomics.h test.c lock.h Makefile
	$(CC) -o test_c11_atomics_spin $(CFLAGS) $(LDFLAGS) -DMY_C11_ATOMICS=1 test.c c11_atomics_spin.c -lpthread

test_c11_atomics_yield: c11_atomics_yield.c c11_atomics.h test.c lock.h Makefile
	$(CC) -o test_c11_atomics_yield $(CFLAGS) $(LDFLAGS) -DMY_C11_ATOMICS=1 test.c c11_atomics_yield.c -lpthread

clean:
	rm -f $(TARGETS) lock_file seg