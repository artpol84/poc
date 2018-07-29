all: test_ex test_so

test_ex: main.c lib.c  lib.h
	gcc -o test_ex main.c lib.c -O3 -g

test_so: main.c lib.c  lib.h
	gcc -c -Wall -Werror -fpic lib.c
	gcc -shared -o libtest.so lib.o
	gcc -o test_so main.c -L. -ltest -Wl,-rpath,$(PWD) -O3 -g

clean:
	rm -f test_so test_ex libtest.so lib.o