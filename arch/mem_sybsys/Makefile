CFLAGS = -g -O3 -I.
all: mem_subs

SRCS = platform.c utils.c ucx_mem_bw.c exec_infra.c basic_access.c main.c
HDRS = platform.h utils.h ucx_mem_bw.h exec_infra.h basic_access.h

SRCS += arch_x86.c
HDRS += arch_x86.h arch_arm.h arch.h

LIB = -lhwloc

# For MacOS
LIB += -I/opt/homebrew/include/ -L/opt/homebrew/lib

mem_subs: $(SRCS) $(HDRS)  
	gcc -o mem_subs $(CFLAGS) $(SRCS) $(LIB)

clean:
	rm -Rf mem_subs