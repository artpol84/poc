
all: main

main: main.c rcu.c rcu.h
	gcc -O0 -g -o main main.c rcu.c -lpthread

clean:
	rm -f main