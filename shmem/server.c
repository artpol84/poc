#include <stdio.h>
#include "sync.h"

int main()
{
    sync_shmem_srv_init();
    while(1) {
        sleep(1);
    }
}
