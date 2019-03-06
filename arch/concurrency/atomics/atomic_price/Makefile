all: atomic_ovh

atomic_ovh: main.c x86.h
	gcc -O3 -g -o atomic_ovh main.c -lpthread -lm

clean:
	rm -f atomic_ovh