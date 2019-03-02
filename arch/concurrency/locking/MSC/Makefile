all: spinlock_ovh msc_ovh

spinlock_ovh: main.c msc_lock.h common.h common.c
	gcc -O3 -g -o spinlock_ovh main.c common.c -lpthread

msc_ovh: main.c msc_lock.h common.h common.c
	gcc -O3 -g -o msc_ovh -DMSC_LOCK main.c common.c -lpthread

clean:
	rm -f spinlock_ovh msc_ovh