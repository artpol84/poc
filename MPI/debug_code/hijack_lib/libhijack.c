#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dlfcn.h>

int PMPI_Finalize( void )
{
    typedef int (*next_t)(void);
    next_t next = dlsym(RTLD_NEXT, "PMPI_Finalize");
    next();
    printf("HIJACK!!!!!!!!!!!!!!!!!!!!\n");
    volatile int delay = 1;
    while( delay ) {
        sleep(1);
    }
    return 0;
}