bin := bruck_test
sources := bruck_test.c

CC := mpicc

LDFLAGS := -lm

.PHONY: clean debug

all: bruck_test.c
	$(CC) $(sources) -o $(bin) $(LDFLAGS) -DDEBUG=0

debug:
	$(CC) $(sources) -o $(bin) $(LDFLAGS) -DDEBUG=1 -g

clean:
	rm -rf $(bin)
