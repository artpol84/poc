branch := branch
branch_opt := branch_opt
branch_objs := branch.o hpctimer.o
branch_opt_objs := branch_opt.o hpctimer.o

CC := gcc
CFLAGS := -O0 -Wall -g -std=c11
LDFLAGS :=

.PHONY: all clean

all: $(branch) $(branch_opt)

$(branch): $(branch_objs)
	$(CC) -o $@ $^ $(LDFLAGS)

$(branch_opt): $(branch_opt_objs)
	$(CC) -o $@ $^ $(LDFLAGS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

#branch.o: branch.c
#branch_opt.o: branch_opt.c
#hpctimer.o: hpctimer.c hpctimer.h

clean:
	@rm -rf *.o $(branch) $(branch_opt)
