CFLAGS = -g -O3 -I.
all: mem_subs

mem_subs: mem_sys.c main.c utils.c arch_x86.c
	gcc -o mem_subs $(CFLAGS) mem_sys.c main.c utils.c arch_x86.c

clean:
	rm -Rf mem_subs