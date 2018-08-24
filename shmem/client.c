#include <stdio.h>
#include "sync.h"

extern int rank;

int main()
{
    int step = 0;
    sync_shmem_cli_init();

    if( rank == 1 ){
        int delay = 0;
        while( delay) {
            sleep(1);
        }
    }

    sync_shmem_do();
    printf("step #%d\n", ++step);

    sync_shmem_do();
    printf("step #%d\n", ++step);

    sync_shmem_do();
    printf("step #%d\n", ++step);

    sync_shmem_do();
    printf("step #%d\n", ++step);

}
