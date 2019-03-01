
all: tslist

tslist: main.c tslist_real.c tslist.h common.h common.c
	gcc -O0 -g -o tslist tslist_real.c main.c common.c -lpthread

clean:
	rm -f tslist