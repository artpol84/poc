#include "common.h"
#include "tslist.h"
#include "x86.h"


pthread_barrier_t tbarrier;
tslist_t *list;

int global_thread_count = 1;
__thread int thread_id = 0;


typedef struct
{
    int thread, seqn;
} data_t;

void *worker(void *tmp)
{
    data_t *data = calloc(nadds, sizeof(data_t));
    int i;

    thread_id = atomic_inc(&global_thread_count, 1);

    bind_to_core(thread_id);

    pthread_barrier_wait(&tbarrier);

    for(i=0; i < nadds; i++) {
        data[i].thread = thread_id;
        data[i].seqn = i;
        tslist_append(list, &data[i]);
    }
}

int main(int argc, char **argv)
{
    int i;
    pthread_t *id;

    process_args(argc,argv);
    bind_to_core(0);

//    ts_base = GET_TS();

    pthread_barrier_init(&tbarrier, NULL, nthreads + 1);

    id = calloc(nthreads, sizeof(*id));
    list = tslist_create();

    /* setup and create threads */
    for (i=0; i<nthreads; i++) {
        pthread_create(&id[i], NULL, worker, (void *)NULL);
    }

    pthread_barrier_wait(&tbarrier);

    for (i=0; i<nthreads; i++) {
        pthread_join(id[i], NULL);
    }

    tslist_elem_t *elem = tslist_first(list);
    int count = 0;
    while( elem ) {
        count++;
        elem = tslist_next(elem);
    }

    printf("Number of elements is %d", count);
}
