#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>  //getopt
#include <string.h>
#include <pthread.h>


typedef enum {
    TEST_P2P_SB_RB,
    TEST_P2P_SNB_RNB,
    TEST_P2P_SB_RNB,
    TEST_P2P_SNB_RB
} test_type_t;

pthread_barrier_t barrier;

test_type_t type;
int thread_mode = 0;
int thread_cnt = 1;
int iter_cnt = 1000;
int test_id = -1;
int want_help = 0;
int debug = 0;

int mpi_rank = -1;
int mpi_size = -1;
int mpi_peer = -1;

#define PDEBUG(tid,fmt,args...)                 \
    if(debug)                                   \
        printf("%d:%d:%s: " fmt " \n",          \
            mpi_rank, tid, __FUNCTION__,        \
            ## args);                           \


void usage(char *progname)
{
    fprintf(stderr, "Usage: %s [parameters]\n", progname);
    fprintf(stderr, "\nParameters are:\n");
    fprintf(stderr, "   -m\tMulti-thread mode\n");
    fprintf(stderr, "   -t\tNumber of threads to use\n");
    fprintf(stderr, "   -n\tNumber of iterations\n");
    fprintf(stderr, "   -v\tTest variant. Available:\n");
    fprintf(stderr, "     \t * p2p-sb-rb\n");
    fprintf(stderr, "     \t * p2p-snb-rnb\n");
    fprintf(stderr, "     \t * p2p-sb-rnb\n");
    fprintf(stderr, "     \t * p2p-snb-rb\n");
    fprintf(stderr, "   -d\tEnable debug\n");
}

test_type_t get_test_type(char *str)
{
    if( !strcmp("p2p-sb-rb", str) ){
        return TEST_P2P_SB_RB;
    }
    if( !strcmp("p2p-snb-rnb", str) ){
        return TEST_P2P_SNB_RNB;
    }
    if( !strcmp("p2p-snb-rb", str) ){
        return TEST_P2P_SNB_RB;
    }
    if( !strcmp("p2p-sb-rnb", str) ){
        return TEST_P2P_SB_RNB;
    }
}

void parse_args(int argc, char **argv)
{
    int c = 0, index = 0;
    opterr = 0;
    while ((c = getopt(argc, argv, "mt:n:v:d")) != -1) {
        switch (c) {
        case 't':
            thread_cnt = atoi(optarg);
            break;
        case 'm':
            thread_mode = 1;
            break;
        case 'n':
            iter_cnt = atoi(optarg);
            break;
        case 'v':
            type = get_test_type(optarg);
            break;
        case 'd':
            debug = 1;
            break;
        case 'h':
        default:
            want_help = 1;
        }
    }
}

void check_args(int argc, char **argv)
{
    char error_str[1024];

    if (want_help) {
        if(0 == mpi_rank) {
            usage(argv[0]);
        }
        goto eexit;
    }

    if( (0 == thread_mode) && (1 < thread_cnt) ) {
        sprintf(error_str, "A single-threaded mode requested with %d (multiple) threads\n", thread_cnt);
        goto eprint;
    }

    if( (0 > thread_mode) ) {
        sprintf(error_str, "MPI_THREAD_MULTIPLE was requested, but it is not supported by MPI implementation\n");
        goto eprint;
    }

    /* All is good */
    return;
eprint:
    if (0 == mpi_rank) {
        printf("ERROR: %s", error_str);
    }
eexit:
    MPI_Finalize();
    exit(1);
}

void* thr_send_b(void *id_ptr)
{
    int tid = *(int*)id_ptr;
    int i;

    pthread_barrier_wait(&barrier);

    PDEBUG(tid,"Start");
    for(i=0; i < iter_cnt; i++) {
        int buf;
        MPI_Send(&buf, 1, MPI_INT, mpi_peer, tid, MPI_COMM_WORLD);
    }
    PDEBUG(tid,"End");
}

void* thr_recv_b(void *id_ptr)
{
    int tid = *(int*)id_ptr;
    int i;

    pthread_barrier_wait(&barrier);

    PDEBUG(tid,"Start");
    for(i=0; i < iter_cnt; i++) {
        int buf;
        MPI_Recv(&buf, 1, MPI_INT, mpi_peer, tid, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    PDEBUG(tid,"End");
}

#define TEST_WIN 50

void* thr_send_nb(void *id_ptr)
{
    int tid = *(int*)id_ptr;
    int i, j;
    MPI_Request req[TEST_WIN];

    pthread_barrier_wait(&barrier);

    PDEBUG(tid,"Start");
    for(i=0; i < iter_cnt; ) {
        int buf;
        for(j = 0; j < TEST_WIN && i < iter_cnt; j++, i++) {
            MPI_Isend(&buf, 1, MPI_INT, mpi_peer, tid, MPI_COMM_WORLD, &req[j]);
        }
        MPI_Waitall(j, req, MPI_STATUSES_IGNORE);
    }
    PDEBUG(tid,"End");
}

void* thr_recv_nb(void *id_ptr)
{
    int tid = *(int*)id_ptr;
    int i, j;
    MPI_Request req[TEST_WIN];

    pthread_barrier_wait(&barrier);

    PDEBUG(tid,"Start");
    for(i=0; i < iter_cnt; ) {
        int buf;
        for(j = 0; j < TEST_WIN && i < iter_cnt; j++, i++) {
            MPI_Irecv(&buf, 1, MPI_INT, mpi_peer, tid, MPI_COMM_WORLD, &req[j]);
        }
        MPI_Waitall(j, req, MPI_STATUSES_IGNORE);
    }
    PDEBUG(tid,"End");
}

typedef void* (*thread_worker_t)(void *id_ptr);

int main(int argc, char **argv)
{
    int mpi_thr_required = 1, mpi_thr_provided = -1, i;
    pthread_t *thread_objs = NULL;
    thread_worker_t *workers = NULL;
    int *thread_ids = NULL;

    /* Parse cmdline arguments.
     * Need to do this prior to MPI init to discover requested threading mode.
     */
    parse_args(argc, argv);

    /* Initialize MPI */
    if (0 == thread_mode) {
        mpi_thr_required = MPI_THREAD_SINGLE;
    } else {
        mpi_thr_required = MPI_THREAD_MULTIPLE;
    }
    MPI_Init_thread(&argc, &argv, mpi_thr_required, &mpi_thr_provided);
    if( mpi_thr_required != mpi_thr_provided ){
        thread_mode = -1;
    }
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    mpi_peer = !mpi_rank;

    pthread_barrier_init(&barrier, NULL, thread_cnt);

    /* Check the arguments and print an error if any */
    check_args(argc, argv);

    workers = calloc(thread_cnt, sizeof(*workers));
    thread_ids = calloc(thread_cnt, sizeof(*thread_ids));
    thread_objs = calloc(thread_cnt, sizeof(*thread_objs));

    switch(type) {
    case TEST_P2P_SB_RB:
        for(i=0; i < thread_cnt; i++){
            if(0 == mpi_rank) {
                /* sender workers */
                workers[i] = thr_send_b;
            } else {
                workers[i] = thr_recv_b;
            }
        }
        break;
    case TEST_P2P_SNB_RNB:
        for(i=0; i < thread_cnt; i++){
            if(0 == mpi_rank) {
                /* sender workers */
                workers[i] = thr_send_nb;
            } else {
                workers[i] = thr_recv_nb;
            }
        }
        break;
    case TEST_P2P_SB_RNB:
        for(i=0; i < thread_cnt; i++){
            if(0 == mpi_rank) {
                /* sender workers */
                workers[i] = thr_send_b;
            } else {
                workers[i] = thr_recv_nb;
            }
        }
        break;
    case TEST_P2P_SNB_RB:
        for(i=0; i < thread_cnt; i++){
            if(0 == mpi_rank) {
                /* sender workers */
                workers[i] = thr_send_nb;
            } else {
                workers[i] = thr_recv_b;
            }
        }
        break;
    }

    for(i=0; i < thread_cnt; i++){
        thread_ids[i] = i;
        pthread_create(&thread_objs[i], NULL, workers[i], &thread_ids[i]);
    }

    for(i=0; i < thread_cnt; i++){
        pthread_join(thread_objs[i], NULL);
    }

    MPI_Finalize();
}

