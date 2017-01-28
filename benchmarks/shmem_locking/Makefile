
CC = mpicc

CFLAGS = -O3 -g
all: test_flock test_pthread test_pthread_prio test_dummy

test_flock: test.c flock.c flock.h lock.h
	$(CC) -o test_flock $(CFLAGS) -DMY_FLOCK=1 test.c flock.c

test_dummy: test.c dummy.c dummy.h lock.h
	$(CC) -o test_dummy $(CFLAGS)  -DMY_DUMMY=1 test.c dummy.c

test_pthread: test.c pthread.c pthread.h lock.h
	$(CC) -o test_pthread $(CFLAGS)  -DMY_PTHREAD=1 test.c pthread.c -lpthread

test_pthread_prio: test.c pthread.c pthread.h lock.h
	$(CC) -o test_pthread_prio $(CFLAGS)  -DMY_PTHREAD=1 -DMY_PTHREAD_PRIO=1 test.c pthread.c -lpthread