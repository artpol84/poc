
all: tslist_cas tslist_fake tslist_swap tslist_swap_deq

CC = gcc
HEADERS = arch_arm.h  arch.h  arch_x86.h  common.h  tslist.h
COMMON_SRC = common.c 
CFLAGS = -O3 -g 

ARCH = $(shell uname -m)
ifeq ($(ARCH),aarch64)
CFLAGS += -march=armv8-a+lse
endif

tslist_cas: main.c tslist_cas.c $(HEADERS) $(COMMON_SRC)
	$(CC) $(CFLAGS) -o tslist_cas tslist_cas.c main.c $(COMMON_SRC) -lpthread

tslist_swap: main.c tslist_swap.c $(HEADERS) $(COMMON_SRC)
	$(CC) $(CFLAGS) -o tslist_swap tslist_swap.c main.c $(COMMON_SRC) -lpthread

tslist_fake: main.c tslist_fake.c $(HEADERS) $(COMMON_SRC)
	$(CC) $(CFLAGS) -o tslist_fake tslist_fake.c main.c $(COMMON_SRC) -lpthread

tslist_swap_deq: main2.c tslist_swap.c $(HEADERS) $(COMMON_SRC)
	$(CC) $(CFLAGS) -o tslist_swap_deq tslist_swap.c main2.c $(COMMON_SRC) -lpthread

clean:
	rm -f tslist_cas tslist_swap tslist_fake tslist_swap_deq