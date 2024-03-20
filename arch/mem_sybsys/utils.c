#include <stdio.h>
#include <stdint.h>

#include "arch.h"
#include "utils.h"


#if 0
/* A core loop that allows to try all kinds of optimizations */
void access(int afirst, int alast, int ifirst, int ilast, int iinc)
{
    int it, a;

    for(it = ifirst; it < ilast; it+=iinc){
        for(a = afirst; a < alast; a++) {
            data[a][it] = it;
        }
    }
}
#endif