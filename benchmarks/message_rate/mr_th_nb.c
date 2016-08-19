/*
 * Copyright (c) 2016      Mellanox Technologies Ltd. ALL RIGHTS RESERVED.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef __USE_GNU
#define __USE_GNU
#endif
#define _GNU_SOURCE

#include <mpi.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>
#include <assert.h>

#include "options.h"
#include "threads.h"
#include "mr_th_nb.h"

//#define DEBUG

settings_t conf;

/* Message rate for each thread */
double *results;
struct thread_info
{
    int tid;
    MPI_Comm comm;
};


/* Split ranks to the pairs communicating with each-other.
 * Following restrictions allpied (may be relaxed if need):
 * - there can only be 2 hosts, error otherwise
 * - number of ranks on each host must be equal
 * otherwise - err exit.
 */
#define MAX_HOSTS 2
#define MAX_HOSTHAME 1024

MPI_Comm split_to_pairs()
{
    int rank, size, len, max_len;
    char hname[MAX_HOSTHAME], *all_hosts, hnames[MAX_HOSTS][MAX_HOSTHAME];
    int i, j, hostnum = 0, *hranks[MAX_HOSTS], hranks_cnt[MAX_HOSTS] = {0, 0};
    MPI_Group base_grp, my_grp;
    MPI_Comm comm, bind_comm;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if( size % 2 ){
        if( rank == 0 ){
            fprintf(stderr,"Expect even number of ranks!\n");
        }
        MPI_Finalize();
        exit(1);
    }

    gethostname(hname, 1024);
    len = strlen(hname) + 1; /* account '\0' */
    MPI_Allreduce(&len, &max_len, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    all_hosts = calloc(sizeof(char), max_len * size);
    MPI_Allgather(hname, max_len, MPI_CHAR, all_hosts, max_len, MPI_CHAR, MPI_COMM_WORLD);

    for(i = 0; i < MAX_HOSTS; i++){
        hranks[i] = calloc(sizeof(int), size);
    }

    for( i = 0; i < size; i++){
        char *base = all_hosts + i * max_len;
        int hidx = -1;
        for(j = 0; j < hostnum; j++){
            if( !strcmp(hnames[j], base) ){
                /* host already known, add rank */
                hidx = j;
                goto save_rank;
            }
        }
        /* new host */
        if( hostnum == 2 ){
            /* too many hosts */
            if( rank == 0 ){
                fprintf(stderr,"Please, launch your application on 2 hosts!\n");
            }
            MPI_Finalize();
            exit(1);
        }
        strcpy(hnames[hostnum], base);
        hidx = hostnum;
        hostnum++;
save_rank:
        hranks[hidx][hranks_cnt[hidx]] = i;
        if( i == rank ){
            conf.my_host_idx = hidx;
            conf.my_rank_idx = hranks_cnt[hidx];
        }
        hranks_cnt[hidx]++;
    }

    /* sanity check, this is already ensured by  */
    if( !conf.intra_node )
        assert( hostnum == 2 );
    else
        assert( hostnum == 1 );

#ifdef DEBUG
    if( 0 == rank ){
        /* output the rank-node mapping */
        for(i=0; i < hostnum; i++){
            int j;
            printf("%s: ", hnames[i]);
            for(j=0; j<hranks_cnt[i]; j++){
                printf("%d ", hranks[i][j]);
            }
            printf("\n");
        }
    }
#endif


    if( !conf.intra_node ){

        /* sanity check */
        if( hranks_cnt[0] != (size / 2) ){
            if( rank == 0 ){
                fprintf(stderr,"Ranks are non-evenly distributed on the nodes!\n");
            }
            MPI_Finalize();
            exit(1);
        }

        /* return my partner */
        conf.i_am_sender = 0;
        if( conf.my_host_idx == 0){
            conf.i_am_sender = 1;
        }
        conf.my_partner = hranks[ (conf.my_host_idx + 1) % MAX_HOSTS ][conf.my_rank_idx];
        conf.my_leader = hranks[ conf.my_host_idx ][0];
    
        /* create the communicator for all senders */
        MPI_Comm_group(MPI_COMM_WORLD, &base_grp);
        MPI_Group_incl(base_grp, size/2, hranks[conf.my_host_idx], &my_grp);
        MPI_Comm_create(MPI_COMM_WORLD, my_grp, &comm);
        /* FIXME: do we need to free it here? Do we need to free base_grp? */
        MPI_Group_free(&my_grp);
        bind_comm = comm;
    } else {
        /* sanity check */
        if( hranks_cnt[0] != size ){
            if( rank == 0 ){
                fprintf(stderr,"Ranks are non-evenly distributed on the nodes!\n");
            }
            MPI_Finalize();
            exit(1);
        }

        /* return my partner */
        conf.i_am_sender = 0;
        if( conf.my_rank_idx % 2 == 0){
            conf.i_am_sender = 1;
        }
        conf.my_partner = hranks[0][ conf.my_rank_idx + ( 1 - 2 * (conf.my_rank_idx % 2)) ];
        conf.my_leader = conf.my_rank_idx % 2;
    
        /* create the communicator for all senders */
        MPI_Comm_split(MPI_COMM_WORLD, conf.my_rank_idx % 2, conf.my_rank_idx, &comm);
        bind_comm = MPI_COMM_WORLD;
    }

    /* discover and exchange binding info */
    setup_binding(bind_comm, &conf);

    /* release the resources */
    free( all_hosts );
    for(i = 0; i < hostnum; i++){
        free( hranks[i] );
    }
    return comm;
}

int main(int argc,char *argv[])
{
    int rank, size, i, mt_level_act;
    char *sthreaded_env;
    pthread_t *id;
    MPI_Comm comm;

    set_default_args(&conf);

    /* unfortunately this is hackish */
    pre_scan_args(argc, argv,&conf);

    if( conf.want_thr_support ){
        MPI_Init_thread(&argc, &argv, MPI_THREAD_MULTIPLE, &mt_level_act);
    } else {
        MPI_Init(&argc, &argv);
        mt_level_act = MPI_THREAD_SINGLE;
    }
    MPI_Comm_size(MPI_COMM_WORLD,&size);
    MPI_Comm_rank(MPI_COMM_WORLD,&rank);

    if (conf.want_thr_support && mt_level_act != MPI_THREAD_MULTIPLE) {
        if( rank == 0 ){
            fprintf(stderr, "NOTE: no thread support!\n");
        }
    }

    process_args(argc, argv, &conf);

    if( conf.threads > 1 && mt_level_act != MPI_THREAD_MULTIPLE ){
        if( rank == 0 ){
            fprintf(stderr, "ERROR: %d threads requested but MPI implementation doesn't support THREAD_MULTIPLE\n",
                    conf.threads);
        }
        MPI_Finalize();
        exit(1);
    }

    comm = split_to_pairs();


    struct thread_info *ti = calloc(conf.threads, sizeof(struct thread_info));
    results = calloc(conf.threads, sizeof(double));
    id = calloc(conf.threads, sizeof(*id));
    sync_init(conf.threads);

    if( conf.threads == 1 ){
        ti[0].tid = 0;
        if( conf.dup_comm ) {
            MPI_Comm_dup(MPI_COMM_WORLD, &ti[i].comm );
        } else {
            ti[i].comm = MPI_COMM_WORLD;
        }
        sync_force_single();
        worker((void*)ti);
    } else {
        /* Create the zero'ed array of ready flags for each thread */
        WMB();

        /* setup and create threads */
        for (i=0; i<conf.threads; i++) {
            ti[i].tid = i;
            if( conf.dup_comm ) {
                MPI_Comm_dup(MPI_COMM_WORLD, &ti[i].comm );
            } else {
                ti[i].comm = MPI_COMM_WORLD;
            }
            pthread_create(&id[i], NULL, worker, (void *) &ti[i]);
        }

        sync_master();

        /* wait for the test to finish */
        for (i=0; i<conf.threads; i++)
            pthread_join(id[i], NULL);
    }

#ifdef DEBUG
    char tmp[1024] = "";
    sprintf(tmp, "%d: ", rank);
    for(i=0; i<conf.threads; i++){
        sprintf(tmp, "%s%lf ", tmp, results[i]);
    }
    printf("%s\n", tmp);
#endif

    if ( conf.i_am_sender ){
        /* FIXME: for now only count on sender side, extend if makes sense */
        double results_rank = 0, results_node = 0;

        for(i=0; i<conf.threads; i++){
            results_rank += results[i];
        }
        MPI_Reduce(&results_rank, &results_node, 1, MPI_DOUBLE, MPI_SUM, conf.my_leader, comm);

        if( conf.my_rank_idx == 0 ){
            printf("%d\t%lf\n", conf.msg_size, results_node);
        }
    }

    if( conf.dup_comm ){
        for (i=0; i<conf.threads; i++) {
            MPI_Comm_free(&ti[i].comm);
        }
    }

    free(id);
    free(results);
    free(ti);

    MPI_Finalize();
    return 0;
}

/*
 * Derived from OSU message rate test:
 * URL: http://mvapich.cse.ohio-state.edu/benchmarks/
 */
void *worker_nb(void *info) {
    struct thread_info *tinfo = (struct thread_info*)info;
    int rank, tag, i, j;
    double stime, etime;
    char *databuf = NULL, syncbuf[4];
    MPI_Request request[ conf.win_size ];
    MPI_Status  status[ conf.win_size ];

    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    databuf = calloc(sizeof(char), conf.msg_size);
    tag = tinfo->tid;
    bind_worker(tag, &conf);

    /* start the benchmark */
    if ( conf.i_am_sender ) {
        for (i=0; i < (conf.iterations + conf.warmup); i++) {
            if ( i == conf.warmup ){
                /* ensure that all threads start "almost" together */
                sync_worker(tag);
                stime = MPI_Wtime();
            }
            for(j=0; j<conf.win_size; j++){
                MPI_Isend(databuf, conf.msg_size, MPI_BYTE, conf.my_partner, tag, tinfo->comm, &request[ j ]);
            }
            MPI_Waitall(conf.win_size, request, status);
            MPI_Recv(syncbuf, 4, MPI_BYTE, conf.my_partner, tag, tinfo->comm, MPI_STATUS_IGNORE);
        }
    } else {
        for (i=0; i< (conf.iterations + conf.warmup); i++) {
            if( i == conf.warmup ){
                /* ensure that all threads start "almost" together */
                sync_worker(tag);
                stime = MPI_Wtime();
            }
            for(j=0; j<conf.win_size; j++){
                MPI_Irecv(databuf, conf.msg_size, MPI_BYTE, conf.my_partner, tag, tinfo->comm, &request[ j ]);
            }
            MPI_Waitall(conf.win_size, request, status);
            MPI_Send(syncbuf, 4, MPI_BYTE, conf.my_partner, tag, tinfo->comm);
        }
    }
    etime = MPI_Wtime();

    results[tag] = 1 / ((etime - stime)/ (conf.iterations * conf.win_size) );
    free(databuf);
    return 0;
}

/*
 * Derived from the threaded test written by R. Thakur and W. Gropp:
 * URL: http://www.mcs.anl.gov/~thakur/thread-tests/
 */
void *worker_b(void *info) {
    struct thread_info *tinfo = (struct thread_info*)info;
    int rank, tag, i, j;
    double stime, etime, ttime;
    char *databuf, syncbuf;
    int nsteps = ( conf.iterations + conf.warmup ) * conf.win_size;

    MPI_Comm_rank(MPI_COMM_WORLD,&rank);
    databuf = calloc(sizeof(char), conf.msg_size);
    tag = tinfo->tid;
    bind_worker(tag, &conf);

    if ( conf.i_am_sender ) {
        for (i=0; i < conf.iterations + conf.warmup; i++) {
            if( i == conf.warmup ){
                /* ensure that all threads start "almost" together */
                sync_worker(tag);
                stime = MPI_Wtime();
            }
            for(j=0; j<conf.win_size; j++){
                MPI_Send(databuf, conf.msg_size, MPI_BYTE, conf.my_partner, tag, MPI_COMM_WORLD);
            }
            MPI_Recv(&syncbuf, 0, MPI_BYTE, conf.my_partner, tag, MPI_COMM_WORLD,
                     MPI_STATUS_IGNORE);
        }
    }
    else {
        for (i=0; i < conf.iterations + conf.warmup; i++) {
            if( i == conf.warmup ){
                /* ensure that all threads start "almost" together */
                sync_worker(tag);
                stime = MPI_Wtime();
            }
            for(j=0; j<conf.win_size; j++){
                MPI_Recv(databuf, conf.msg_size, MPI_BYTE, conf.my_partner, tag, MPI_COMM_WORLD,
                         MPI_STATUS_IGNORE);
            }
            MPI_Send(&syncbuf, 0, MPI_BYTE, conf.my_partner, tag, MPI_COMM_WORLD);
        }
    }
    etime = MPI_Wtime();
    results[tag] = 1 / ((etime - stime)/ (conf.iterations * conf.win_size) );
    return 0;
}
