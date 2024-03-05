CFLAGS = -g -O3 -I.
all: mem_subs

mem_subs: mem_sys.c main.c
	gcc -o mem_subs $(CFLAGS) mem_sys.c main.c

clean:
	rm -Rf mem_subs