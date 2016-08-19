#include <mpi.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>

#include "options.h"

void set_default_args(settings_t *s)
{
    memset(s, 0, sizeof(*s));
    s->worker = worker_nb;
    s->threads = 1;
    s->win_size = 256;
    s->iterations = 100;
    s->warmup = 10;
    s->my_partner = -1;
    s->i_am_sender = -1;
    s->my_host_idx = -1;
    s->my_rank_idx = -1;
    s->my_leader = -1;
}

static void usage(char *cmd)
{
    settings_t s;
    set_default_args(&s);
    fprintf(stderr, "Options: %s\n", cmd);
    fprintf(stderr, "\t-h\tDisplay this help\n");
    fprintf(stderr, "Test description:\n");
    fprintf(stderr, "\t-s\tMessage size (default: %d)\n", s.msg_size);
    fprintf(stderr, "\t-n\tNumber of measured iterations (default: %d)\n", s.iterations);
    fprintf(stderr, "\t-w\tNumber of warmup iterations (default: %d)\n", s.warmup);
    fprintf(stderr, "\t-W\tWindow size - number of send/recvs between sync (default: %d)\n", s.win_size);
    fprintf(stderr, "Test options:\n");
    fprintf(stderr, "\t-Dthrds\tDisable threaded support (call MPI_Init) (default: %s)\n", 
            (s.want_thr_support)  ? "MPI_Init_thread" : "MPI_Init");
    fprintf(stderr, "\t-t\tNumber of threads (default: %d)\n", s.threads);
    fprintf(stderr, "\t-B\tBlocking send (default: %s)\n", (s.worker == worker_b) ? "blocking" : "non-blocking");
    fprintf(stderr, "\t-S\tSMP mode - intra-node performance - pairwise exchanges (default: %s)\n", 
            (s.intra_node) ? "enabled" : "disabled" );
    fprintf(stderr, "\t-d\tUse separate communicator for each thread (default: %s)\n", (s.dup_comm) ? "enabled" : "disabled");
    fprintf(stderr, "\t-b\tEnable fine-grained binding of the threads inside the set provided by MPI\n");
    fprintf(stderr, "\t\tbenchmark is able to discover node-local ranks and exchange existing binding\n");
    fprintf(stderr, "\t\t(default: %s)\n", (s.binding) ? "enabled" : "disabled");
}

int check_unsigned(char *str)
{
    return (strlen(str) == strspn(str,"0123456789") );
}

void pre_scan_args(int argc, char **argv, settings_t *s)
{
    int i;
    /* this is a hack - you cannot access argc/argv before a call to MPI_Init[_thread].
     * But it seems to work with most modern MPIs: Open MPI and Intel MPI was checked
     */
    for(i=0; i < argc; i++){
        if( 0 == strcmp(argv[i], "-Dthrds") ){
            s->want_thr_support = 0;
        }
    }
}

int process_args(int argc, char **argv, settings_t *s)
{
    int c, rank;

    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    while((c = getopt(argc, argv, "hD:t:BbW:n:w:ds:S")) != -1) {
        switch (c) {
        case 'h':
            if( 0 == rank ){
                usage(argv[0]);
            }
            MPI_Finalize();
            exit(0);
            break;
        case 'D':
            /* from the getopt perspective thrds is an optarg. Check that 
             * user haven't specified anything else */
            if( strcmp( optarg, "thrds" ) ){
                goto error;
            }
            break;
        case 't':
            if( !check_unsigned(optarg) ) {
                goto error;
            }
            s->threads = atoi(optarg);
            if( s->threads == 0 ){
                goto error;
            }
            break;
        case 'B':
            s->worker = worker_b;
            break;
        case 'b':
            s->binding = 1;
            break;
        case 'W':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            s->win_size = atoi(optarg);
            if( s->win_size == 0 ){
                goto error;
            }
            break;
        case 'n':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            s->iterations = atoi(optarg);
            if( s->iterations == 0 ){
                goto error;
            }
            break;
        case 'w':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            s->warmup = atoi(optarg);
            if( s->warmup == 0 ){
                goto error;
            }
            break;
        case 's':
            if( !check_unsigned(optarg) ){
                goto error;
            }
            s->msg_size = atoi(optarg);
            if( s->msg_size == 0 ){
                goto error;
            }
            break;
        case 'd':
            s->dup_comm = 1;
            break;
        case 'S':
            s->intra_node = 1;
            break;
        default:
            c = -1;
            goto error;
        }
    }
    return;
error:
    if(0 == rank) {
        if( c != -1 ){
            fprintf(stderr, "Bad argument of '-%c' option\n", (char)c);
        }
        
        usage(argv[0]);
    }
    MPI_Finalize();
    exit(1);
}
