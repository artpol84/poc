
all: tslist_cas tslist_fake tslist_swap tslist_swap_deq

tslist_cas: main.c tslist_cas.c tslist.h common.h common.c
	gcc -O0 -g -o tslist_cas tslist_cas.c main.c common.c -lpthread

tslist_swap: main.c tslist_swap.c tslist.h common.h common.c
	gcc -O0 -g -o tslist_swap tslist_swap.c main.c common.c -lpthread

tslist_fake: main.c tslist_fake.c tslist.h common.h common.c
	gcc -O0 -g -o tslist_fake tslist_fake.c main.c common.c -lpthread

tslist_swap_deq: main2.c tslist_swap.c tslist.h common.h common.c
	gcc -O0 -g -o tslist_swap_deq tslist_swap.c main2.c common.c -lpthread

clean:
	rm -f tslist_cas tslist_swap tslist_fake tslist_swap_deq