#include <stdio.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h> 

atomic_int acnt;
int cnt;
 
void *f(void* thr_data)
{
    for(int n = 0; n < 100000; ++n) {
        _Bool flag = false;
        while( !flag ){
            int tmp = acnt;
            flag = atomic_compare_exchange_strong(&acnt, &tmp, tmp + 1);
        }
        ++cnt; // undefined behavior, in practice some updates missed
    }
    return 0;
}
 
int main(void)
{
    pthread_t thr[10];

    for(int n = 0; n < 10; ++n)
        pthread_create(&thr[n], NULL, f, NULL);
    for(int n = 0; n < 10; ++n)
        pthread_join(thr[n], NULL);
 
    printf("The atomic counter is %u\n", acnt);
    printf("The non-atomic counter is %u\n", cnt);
}
