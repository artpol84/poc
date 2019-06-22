#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <unistd.h>  //getopt
#include <string.h>
#include <pthread.h>

#define GET_TS() ({                         \
    struct timespec ts;                     \
    double ret = 0;                         \
    clock_gettime(CLOCK_MONOTONIC, &ts);    \
    ret = ts.tv_sec + 1E-9*ts.tv_nsec;      \
    ret;                                    \
})

typedef enum {
    TEST_NONE,
    TEST_P2P_SB_RB,
    TEST_P2P_SNB_RNB,
    TEST_P2P_SB_RNB,
    TEST_P2P_SNB_RB,
    TEST_COLL_BARRIER
} test_type_t;

pthread_barrier_t barrier;

test_type_t type = TEST_P2P_SB_RB;
int thread_mode = 0;
int thread_cnt = 1;
int iter_cnt = 1000;
int test_id = -1;
int want_help = 0;
int debug = 0;

int mpi_rank = -1;
int mpi_size = -1;
int mpi_peer = -1;

typedef enum {
    TEST_MPI_SEND,
    TEST_MPI_RECV,
    TEST_MPI_ISEND,
    TEST_MPI_IRECV,
    TEST_MPI_WAITALL,
    TEST_MPI_BARRIER,
    TEST_MPI_COUNT
} test_mpi_call_ids_t;

char *mpi_api_names[TEST_MPI_COUNT] =
{
    [TEST_MPI_SEND] = "MPI_Send",
    [TEST_MPI_RECV] = "MPI_Recv",
    [TEST_MPI_ISEND] = "MPI_Isend",
    [TEST_MPI_IRECV] = "MPI_Irecv",
    [TEST_MPI_WAITALL] = "MPI_Waitall",
    [TEST_MPI_BARRIER] = "MPI_Barrier",
};

typedef struct {
    int count;
    double total_time;
} test_mpi_call_stat_t;

test_mpi_call_stat_t *stats_thr[TEST_MPI_COUNT] = { 0 };
test_mpi_call_stat_t stats_glob[TEST_MPI_COUNT] = { 0 };
MPI_Comm *coll_comms = NULL;

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
    fprintf(stderr, "     \t * coll-barrier\n");
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
    if( !strcmp("coll-barrier", str) ){
        return TEST_COLL_BARRIER;
    }
    return TEST_NONE;
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

    if (TEST_NONE == type) {
        sprintf(error_str, "Unknown test type request\n");
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
    double start;

    pthread_barrier_wait(&barrier);

    PDEBUG(tid,"Start");
    start = GET_TS();
    for(i=0; i < iter_cnt; i++) {
        int buf;
        MPI_Send(&buf, 1, MPI_INT, mpi_peer, tid, MPI_COMM_WORLD);
    }
    stats_thr[TEST_MPI_SEND][tid].count += iter_cnt;
    stats_thr[TEST_MPI_SEND][tid].total_time += GET_TS() - start;
    PDEBUG(tid,"End");
}

void* thr_recv_b(void *id_ptr)
{
    int tid = *(int*)id_ptr;
    int i;
    double start;

    pthread_barrier_wait(&barrier);

    PDEBUG(tid,"Start");
    start = GET_TS();
    for(i=0; i < iter_cnt; i++) {
        int buf;
        MPI_Recv(&buf, 1, MPI_INT, mpi_peer, tid, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }
    stats_thr[TEST_MPI_RECV][tid].count += iter_cnt;
    stats_thr[TEST_MPI_RECV][tid].total_time += GET_TS() - start;
    PDEBUG(tid,"End");
}

#define TEST_WIN 50

void* thr_send_nb(void *id_ptr)
{
    int tid = *(int*)id_ptr;
    int i, j;
    double start;
    MPI_Request req[TEST_WIN];

    pthread_barrier_wait(&barrier);

    PDEBUG(tid,"Start");
    for(i=0; i < iter_cnt; ) {
        int buf;
        start = GET_TS();
        for(j = 0; j < TEST_WIN && i < iter_cnt; j++, i++) {
            MPI_Isend(&buf, 1, MPI_INT, mpi_peer, tid, MPI_COMM_WORLD, &req[j]);
        }
        stats_thr[TEST_MPI_ISEND][tid].count += j;
        stats_thr[TEST_MPI_ISEND][tid].total_time += GET_TS() - start;

        start = GET_TS();
        MPI_Waitall(j, req, MPI_STATUSES_IGNORE);
        stats_thr[TEST_MPI_WAITALL][tid].count += 1;
        stats_thr[TEST_MPI_WAITALL][tid].total_time += GET_TS() - start;
    }

    PDEBUG(tid,"End");
}

void* thr_recv_nb(void *id_ptr)
{
    int tid = *(int*)id_ptr;
    int i, j;
    double start;
    MPI_Request req[TEST_WIN];

    pthread_barrier_wait(&barrier);

    PDEBUG(tid,"Start");
    for(i=0; i < iter_cnt; ) {
        int buf;
        start = GET_TS();
        for(j = 0; j < TEST_WIN && i < iter_cnt; j++, i++) {
            MPI_Irecv(&buf, 1, MPI_INT, mpi_peer, tid, MPI_COMM_WORLD, &req[j]);
        }
        stats_thr[TEST_MPI_IRECV][tid].count += j;
        stats_thr[TEST_MPI_IRECV][tid].total_time += GET_TS() - start;

        start = GET_TS();
        MPI_Waitall(j, req, MPI_STATUSES_IGNORE);
        stats_thr[TEST_MPI_WAITALL][tid].count += 1;
        stats_thr[TEST_MPI_WAITALL][tid].total_time += GET_TS() - start;

    }
    PDEBUG(tid,"End");
}

void* thr_barrier(void *id_ptr)
{
    int tid = *(int*)id_ptr;
    int i, j;
    MPI_Request req[TEST_WIN];
    double start;

    pthread_barrier_wait(&barrier);

    PDEBUG(tid,"Start");
    start = GET_TS();
    for(i=0; i < iter_cnt; i++) {
        MPI_Barrier(coll_comms[tid]);
    }
    stats_thr[TEST_MPI_BARRIER][tid].count += iter_cnt;
    stats_thr[TEST_MPI_BARRIER][tid].total_time += GET_TS() - start;
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

    for(i=0; i < TEST_MPI_COUNT; i++) {
        stats_thr[i] = calloc(thread_cnt, sizeof(*stats_thr[i]));
    }
    workers = calloc(thread_cnt, sizeof(*workers));
    thread_ids = calloc(thread_cnt, sizeof(*thread_ids));
    thread_objs = calloc(thread_cnt, sizeof(*thread_objs));

    if (type >= TEST_COLL_BARRIER) {
        /* If the test is using collectives - need to create communicators */
        coll_comms = calloc(thread_cnt, sizeof(*coll_comms));
        for (i=0; i<thread_cnt; i++) {
            MPI_Comm_dup(MPI_COMM_WORLD, &coll_comms[i]);
        }
    }

    switch (type) {
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
    case TEST_COLL_BARRIER:
        for(i=0; i < thread_cnt; i++){
            workers[i] = thr_barrier;
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

    memset(stats_glob, 0, sizeof(stats_glob));
    for(i=0; i<TEST_MPI_BARRIER; i++){
        test_mpi_call_stat_t buf[mpi_size];
        int j;
        for(j=0; j<thread_cnt; j++) {
            stats_glob[i].count += stats_thr[i][j].count;
            stats_glob[i].total_time += stats_thr[i][j].total_time;
        }
        MPI_Gather(&stats_glob[i], sizeof(stats_glob[i]), MPI_CHAR,
                   buf, sizeof(stats_glob[i]), MPI_CHAR,
                   0, MPI_COMM_WORLD);
        for(j=1; j < mpi_size; j++) {
            stats_glob[i].count += buf[j].count;
            stats_glob[i].total_time += buf[j].total_time;
        }
    }

    if (0 == mpi_rank) {
        printf("Tested MPI API statistics:\n");
        for(i=0; i<TEST_MPI_BARRIER; i++){
            if (!stats_glob[i].count) {
                continue;
            }
            printf("%s: count=%d, total_time=%lf\n",
                   mpi_api_names[i], stats_glob[i].count,
                   stats_glob[i].total_time);
        }
        printf("%s: count=%d, <service calls>\n",
               "MPI_Gather", TEST_MPI_COUNT);
    }

    free(workers);
    free(thread_ids);
    free(thread_objs);
    if (NULL != coll_comms) {
        for (i=0; i<thread_cnt; i++) {
            MPI_Comm_free(&coll_comms[i]);
        }
        free(coll_comms);
    }
    for(i=0; i < TEST_MPI_COUNT; i++) {
        free(stats_thr[i]);
    }

    MPI_Finalize();

}

