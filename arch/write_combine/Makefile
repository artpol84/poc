CFLAGS = -g -O3
all: wc_bench

wc_bench: wc_bench.c
	gcc -o wc_bench $(CFLAGS) wc_bench.c

clean:
	rm -Rf wc_bench