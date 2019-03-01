#ifndef __USE_GNU
#define __USE_GNU
#endif
#define _GNU_SOURCE

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <assert.h>

#include "x86_64.h"
#include "rcu.h"

#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
    })

#define DEFAULT_NTHREADS 1
int nthreads, niters;
pthread_barrier_t tbarrier;
rcu_lock_t rcu;
float write_prob;

typedef struct list_s {
    int *ptr;
    struct list_s *next;
} list_t;

extern __thread unsigned long *tls_cntr;

list_t *list;
int _list_size = 0;
double ts_base = 0;

void set_default_args()
{
    nthreads = DEFAULT_NTHREADS;
    write_prob = 0.05;
}

void usage(char *cmd)
{
    set_default_args();
    fprintf(stderr, "Options: %s\n", cmd);
    fprintf(stderr, "\t-h\tDisplay this help\n");
    fprintf(stderr, "\t-n\tNumber of threads (default: %d)\n", nthreads);
    fprintf(stderr, "\t-i\tNumber of iterations (default: %d)\n", niters);
    fprintf(stderr, "\t-p\tWrite probability (default: %f)\n", write_prob);
}

int check_unsigned(char *str)
{
    return (strlen(str) == strspn(str,"0123456789") );
}

int check_float(char *str)
{
    char *ptr = NULL;
    ptr = strchr(str, '.');
    if(NULL == ptr) {
        return check_unsigned(str);
    } else {
        *ptr = '\0';
        int flag = check_unsigned(str) && check_unsigned(ptr + 1);
        *ptr = '.';
        return flag;
    }
}

void process_args(int argc, char **argv)
{
    int c, rank;

    set_default_args();

    while((c = getopt(argc, argv, "hn:i:p:")) != -1) {
        printf("Process option '%c'\n", c);
        switch (c) {
        case 'h':
            usage(argv[0]);
            exit(0);
            break;
        case 'n':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            nthreads = atoi(optarg);
            printf("nthreads = %d\n", nthreads);
            if (0 >= nthreads) {
                goto error;
            }
            break;
        case 'i':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            niters = atoi(optarg);
            printf("niters = %d\n", niters);
            if (0 >= niters) {
                goto error;
            }
            break;
        case 'p':
            if( !check_float(optarg) ){
                goto error;
            }
            sscanf(optarg,"%f", &write_prob);
            if ((0 >= write_prob) || (1 < write_prob)) {
                goto error;
            }
            break;
        default:
            c = -1;
            goto error;
        }
    }
    return;
error:
    if( c != -1 ){
        fprintf(stderr, "Bad argument of '-%c' option\n", (char)c);
    }
    usage(argv[0]);
    exit(1);
}

void bind_to_core(int thr_idx)
{
    long configured_cpus = 0;
    cpu_set_t set;
    int error_loc = 0, error;
    int ncpus;

    /* How many cpus we have on the node */
    configured_cpus = sysconf(_SC_NPROCESSORS_CONF);

    if( sizeof(set) * 8 < configured_cpus ){
        /* Shouldn't happen soon, currentlu # of cpus = 1024 */
        if( 0 == thr_idx ){
            fprintf(stderr, "The size of CPUSET is larger that we can currently handle\n");
        }
        exit(1);
    }

    if( configured_cpus < thr_idx ){
        fprintf(stderr, "ERROR: (ncpus < thr_idx)\n");
        exit(1);
    }

    CPU_ZERO(&set);
    CPU_SET(thr_idx, &set);
    if( pthread_setaffinity_np(pthread_self(), sizeof(set), &set) ){
        abort();
    }
}

void list_traverse(list_t *list)
{
    list_t *elem = list->next;
    int sum;

    while(elem) {
        rmb();
        sum += *elem->ptr;
        elem = elem->next;
    }
}

void list_add(list_t *list)
{
    static int counter = 0;
    list_t *elem = malloc(sizeof(*elem));
    elem->ptr = malloc(sizeof(*elem->ptr));
    *elem->ptr = ++counter;
    elem->next = list->next;
    wmb();
    list->next = elem;
    _list_size++;
}

list_t *list_del(list_t *list)
{
    list_t *elem;
    elem = list->next;
    if( elem ) {
        list->next = elem->next;
    }
    wmb();
    _list_size--;
    return elem;
}

void list_release(list_t *elem)
{
    free(elem->ptr);
    memset(elem, 0xff, sizeof(*elem));
    free(elem);
}

int list_size() { return _list_size; }

void *worker(void *_id) {
    int  id = *((int*)_id), i;
    int seed = id;
    bind_to_core(id);
    pthread_barrier_wait(&tbarrier);
    char name[256];
    sprintf(name, "dump_%d.log", id);
    FILE *fp = fopen(name, "w");
    double ts_event;
    int cntr;

    printf("niters = %d\n", niters);

    rcu_attach(&rcu);

    for(i=0; i < niters; i++) {
        double event = (double)rand_r(&seed) / RAND_MAX;
        ts_event = GET_TS();
        ts_event -= ts_base;
        cntr = rcu.gcntr;

        if( event < write_prob ) {
            double event1 = (double)rand_r(&seed) / RAND_MAX;
	
            list_t *elem = NULL;
            rcu_handler_t hndl;
            int need_completion = 0;
            int type = 0;

            /* Take a writer lock */
            rcu_write_lock(&rcu);

            /* Perform the action */
            if( !list_size(list) ) {
                type = 1;
                list_add(list);
            } else {
                if( event1 < 0.6) {
                    type = 1;
                    list_add(list);
                } else {
                    type = 2;
                    elem = list_del(list);
                    /* Shift to the next update incarnation */
                    hndl = rcu_update_initiate(&rcu);
                    need_completion = 1;
                }
            }
            rcu_write_unlock(&rcu);

            fprintf(fp, "%lf: %s element, icntr = %zu, cntr = %zu, event1 = %lf\n",
                    ts_event, (type == 1) ? "add" : "rem",
                    cntr, rcu.gcntr, event1);

            if( need_completion ) {
                while( !rcu_test_complete(&rcu, hndl) ) {
                    struct timespec ts = { 0, 100000 };
                    nanosleep(&ts, NULL);
                }
            }

            if( elem ) {
                list_release(elem);
            }
        } else {
            unsigned long icntr, ecntr;
            rcu_read_lock(&rcu);
            icntr = *tls_cntr;
            list_traverse(list);
            rcu_read_unlock(&rcu);
            ecntr = *tls_cntr;
            fprintf(fp, "%lf: traverse: icntr = %zu, ecntr = %zu\n",
                    ts_event, icntr, ecntr);

        }
    }
    fclose(fp);
    pthread_barrier_wait(&tbarrier);
}

int main(int argc, char **argv)
{
    int *tids, i;
    pthread_t *id;
    list = malloc(sizeof(*list));
    list->next = NULL;
    list->ptr = NULL;

    process_args(argc,argv);
    bind_to_core(0);

    ts_base = GET_TS();

    pthread_barrier_init(&tbarrier, NULL, nthreads + 1);

    tids = calloc(nthreads, sizeof(*tids));
    id = calloc(nthreads, sizeof(*id));
    rcu_init(&rcu);

    /* setup and create threads */
    for (i=0; i<nthreads; i++) {
        tids[i] = i + 1;
        pthread_create(&id[i], NULL, worker, (void *)&tids[i]);
    }

    pthread_barrier_wait(&tbarrier);

    pthread_barrier_wait(&tbarrier);

    for (i=0; i<nthreads; i++) {
        pthread_join(id[i], NULL);
    }
}
