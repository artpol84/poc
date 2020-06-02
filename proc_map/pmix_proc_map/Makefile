PMIX_INST=/home/user/workspace/openpmix/install

CFLAGS= -Wall -I${PMIX_INST}/include
LDFLAGS= -Wl,-rpath=${PMIX_INST}/lib -g

PMIX_LIB= -L${PMIX_INST}/lib -lpmix

all: pmix_regexp_time

pmix_regexp_time.o:  pmix_regexp_time.c
	gcc ${CFLAGS} -c  pmix_regexp_time.c

pmix_regexp_time:  pmix_regexp_time.o
	gcc ${CFLAGS} ${LDFLAGS} pmix_regexp_time.o ${PMIX_LIB} -o  pmix_regexp_time

clean:
	rm -f  pmix_regexp_time.o  pmix_regexp_time
