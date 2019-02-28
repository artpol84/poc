#include <pthread.h>

int main()
{
    pthread_barrier_t b;
    pthread_barrier_init(&b, NULL, 1);
}