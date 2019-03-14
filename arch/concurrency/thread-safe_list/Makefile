
all: tslist_cas tslist_fake tslist_swap

tslist_cas: main.c tslist_cas.c tslist_cas.h common.h common.c
	gcc -O0 -g -DTSLIST_HEADER="\"tslist_cas.h\"" -o tslist_cas tslist_cas.c main.c common.c -lpthread

tslist_swap: main.c tslist_swap.c tslist_swap.h common.h common.c
	gcc -O0 -g -DTSLIST_HEADER="\"tslist_swap.h\"" -o tslist_swap tslist_swap.c main.c common.c -lpthread

tslist_fake: main.c tslist_fake.c tslist_fake.h common.h common.c
	gcc -O0 -g -DTSLIST_HEADER="\"tslist_fake.h\"" -o tslist_fake tslist_fake.c main.c common.c -lpthread

clean:
	rm -f tslist_cas tslist_swap tslist_fake