all: test_ex test_so

CC = gcc

test_ex: main.c lib.c  lib.h
	$(CC) -o test_ex main.c lib.c -O3 -g -lpthread

test_so: main.c lib.c  lib.h
	$(CC) -c -Wall -Werror -fpic -O3 lib.c
	$(CC) -shared -o libtest.so lib.o
	$(CC) -o test_so main.c -L. -ltest -Wl,-rpath,$(PWD) -O3 -g -lpthread

clean:
	rm -f test_so test_ex libtest.so lib.o