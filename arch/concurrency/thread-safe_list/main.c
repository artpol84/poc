#include "common.h"
#include "tslist.h"
#include "x86.h"


pthread_barrier_t tbarrier;
tslist_t *list;

int global_thread_count = -1;
__thread int thread_id = -1;


typedef struct
{
    int thread, seqn;
} data_t;

void *worker(void *tmp)
{
    data_t *data = calloc(nadds * nbatch, sizeof(data_t));
    void *ptrs[nbatch];
    int i, j;

    thread_id = atomic_inc(&global_thread_count, 1);

    bind_to_core(thread_id);

    pthread_barrier_wait(&tbarrier);

    for(i=0; i < nadds; i++) {
        for(j = 0; j < nbatch; j++) {
            int idx = i * nbatch + j;
            data[idx].thread = thread_id;
            data[idx].seqn = i;
            ptrs[j] = &data[idx];
        }
        tslist_append_batch(list, ptrs, nbatch);
    }
}

int main(int argc, char **argv)
{
    int i;
    pthread_t *id;
    double start, time = 0;

    process_args(argc,argv);
//    bind_to_core(0);

    pthread_barrier_init(&tbarrier, NULL, nthreads + 1);

    id = calloc(nthreads, sizeof(*id));
    list = tslist_create();

    /* setup and create threads */
    for (i=0; i<nthreads; i++) {
        pthread_create(&id[i], NULL, worker, (void *)NULL);
    }

    pthread_barrier_wait(&tbarrier);
    start = GET_TS();

    for (i=0; i<nthreads; i++) {
        pthread_join(id[i], NULL);
    }
    time = GET_TS() - start;

    tslist_elem_t *elem = tslist_first(list);
    int count = 0;
    while( elem ) {
        count++;
        elem = tslist_next(elem);
    }
    printf("Number of elements is %d\n", count);
    printf("Time: %lf\n", time * 1E6);
    printf("Performance: %lf Melem/s\n", count / time / 1E6);
/*
    elem = tslist_first(list);
    while( elem ) {
        elem = tslist_next(elem);
        if( elem ) {
            data_t *ptr = elem->ptr;
            printf("(%d;%d) ", ptr->thread, ptr->seqn);
        }
    }
    printf("\n");
*/
}
