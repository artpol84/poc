all: lock_ovh

lock_ovh: main.c msc_lock.h common.h common.c
	gcc -O3 -g -o lock_ovh main.c common.c -lpthread

clean:
	rm -f lock_ovh