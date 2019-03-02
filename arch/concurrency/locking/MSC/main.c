#include "common.h"
#include "msc_lock.h"

//#define MSC_LOCK
#ifdef MSC_LOCK
msc_lock_t lock;
static void lock_init()
{
    msc_init(&lock);
}

static void lock_destroy()
{
    msc_destroy(&lock);
}

static void lock_lock(int id)
{
    msc_lock(&lock, id);
}

static void lock_unlock(int id)
{
    msc_unlock(&lock, id);
}
#else
pthread_spinlock_t lock;
static void lock_init()
{
    pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);
}

static void lock_destroy()
{
    pthread_spin_destroy(&lock);
}

static void lock_lock(int id)
{
    pthread_spin_lock(&lock);
}

static void lock_unlock(int id)
{
    pthread_spin_unlock(&lock);
}
#endif

pthread_barrier_t tbarrier;
int global_counter = 0;
// NOTE we limit experiment to 1K threads
// This is a POC so it doesn't matter
double performance_data[1024] = { 0 };

void *worker(void *_id)
{
    int i, tid = *((int*)_id);
    int local_counter = 0;
    bind_to_core(tid + 1);

    pthread_barrier_wait(&tbarrier);

    double start = GET_TS();
    for(i=0; i < niter; i++) {
        lock_lock(tid);
        if( verify_mode ) {
            // In the verification mode we want to make sure that
            // global counter won't miss additions
            global_counter++;
        } else {
            // In the performance measurement mode we don't want additional
            // cache invalidations, so deal with the local variable.
            local_counter++;
        }
        lock_unlock(tid);
    }
    performance_data[tid] = GET_TS() - start;
//    sleep(100000);
}

int main(int argc, char **argv)
{
    int i;

    process_args(argc,argv);
    bind_to_core(0);

    pthread_barrier_init(&tbarrier, NULL, nthreads + 1);
    int tids[nthreads];
    pthread_t id[nthreads];

    lock_init();

    /* setup and create threads */
    for (i=0; i<nthreads; i++) {
        tids[i] = i;
        pthread_create(&id[i], NULL, worker, (void *)&tids[i]);
    }

    pthread_barrier_wait(&tbarrier);

    for (i=0; i<nthreads; i++) {
        pthread_join(id[i], NULL);
    }

    lock_destroy();

    double sum = 0;
    for(i=0; i < nthreads; i++) {
        sum += performance_data[i];
    }

    printf("Average latency / lock acquire: %lf us\n", 1E6 * sum / (nthreads * niter));
    
    if( verify_mode ){
	if( global_counter != nthreads * niter ) {
	    printf("Verification: FAILED!\n");
	} else {
	    printf("Verification: SUCCESS!\n");
	}
    }

}
