#ifndef __USE_GNU
#define __USE_GNU
#endif
#define _GNU_SOURCE

#include <mpi.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sched.h>

#include "threads.h"

/* thread synchronization */
static int volatile sync_cur_step = 1;
static int volatile *sync_thread_ready = NULL;
static int volatile sync_start_all = 0;
static int nthreads = 0;

void sync_init(int _nthreads)
{
    nthreads = _nthreads;
    sync_thread_ready = calloc(nthreads, sizeof(int));
}

void sync_force_single()
{
    sync_start_all = sync_cur_step;
}

void sync_worker(int tid)
{
    int cur_step = sync_cur_step;
    WMB();
    /* now we are ready to send */
    sync_thread_ready[tid] = sync_cur_step;
    WMB();
    /* wait for everybody */
    while( sync_start_all != cur_step ){
        usleep(1);
    }
}

void sync_master()
{
    int i;
    int cur_step = sync_cur_step;
    /* wait for all threads to start */
    WMB();

    for(i=0; i<nthreads; i++){
        while( sync_thread_ready[i] != cur_step ){
            usleep(10);
        }
    }

    /* Synchronize all processes */
    WMB();
    MPI_Barrier(MPI_COMM_WORLD);
    sync_cur_step++;
    /* signal threads to start */
    WMB();
    sync_start_all = cur_step;
}


static char *binding_err_msgs[] = {
    "all is OK",
    "some nodes have more procs than CPUs. Oversubscription is not supported by now",
    "some process have overlapping but not equal bindings. Not supported by now",
    "some process was unable to read it's affinity",
};

enum bind_errcodes {
    BIND_OK,
    BIND_OVERSUBS,
    BIND_OVERLAP,
    BIND_GETAFF
};


/* binding-related */
long configured_cpus;
cpu_set_t my_cpuset;

void setup_binding(MPI_Comm comm, settings_t *s)
{
    int grank;
    cpu_set_t set, *all_sets;
    int *ranks, ranks_cnt = 0, my_idx = -1;
    int rank, size, ncpus;
    int error_loc = 0, error;
    int cpu_no, cpus_used = 0;
    int i;

    if( !s->binding ){
        return;
    }

    /* How many cpus we have on the node */
    configured_cpus = sysconf(_SC_NPROCESSORS_CONF);

    /* MPI identifications */
    MPI_Comm_rank(MPI_COMM_WORLD, &grank);
    MPI_Comm_rank(comm, &rank);
    MPI_Comm_size(comm, &size);
    ranks = calloc(size, sizeof(*ranks));

    if( sizeof(set) * 8 < configured_cpus ){
        /* Shouldn't happen soon, currentlu # of cpus = 1024 */
        if( 0 == grank ){
            fprintf(stderr, "The size of CPUSET is larger that we can currently handle\n");
        }
        MPI_Finalize();
        exit(1);
    }

    if( sched_getaffinity(0, sizeof(set), &set) ){
        error_loc = BIND_GETAFF;
    }

    ncpus = CPU_COUNT(&set);

    /* FIXME: not portable, do we care about heterogenious systems? */
    all_sets = calloc( size, sizeof(set));
    MPI_Allgather(&set, sizeof(set), MPI_BYTE, all_sets, sizeof(set), MPI_BYTE, comm);


    if( error_loc ){
        goto finish;
    }

    /* find procs that are bound exactly like we are.
     * Note that we don't expect overlapping set's:
     *  - either they are the same;
     *  - or disjoint.
     */
    for(i=0; i<size; i++){
        cpu_set_t *rset = all_sets + i, cmp_set;
        CPU_AND(&cmp_set, &set, rset);
        if( CPU_COUNT(&cmp_set) == ncpus ){
            /* this rank is binded as we are */
            ranks[ ranks_cnt++ ] = i;
        } else if( !CPU_COUNT(&cmp_set) ){
            /* other binding */
            continue;
        } else {
            /* not expected. Error exit! */
            error_loc = BIND_OVERLAP;
            goto finish;
        }
    }

    if( ncpus < ranks_cnt * s->threads ){
        error_loc = BIND_OVERSUBS;
    }

    for(i=0; i<ranks_cnt; i++){
        if( ranks[i] == rank ){
            my_idx = i;
            break;
        }
    }
    /* sanity check */
    assert(my_idx >= 0);

    cpu_no = 0;
    CPU_ZERO(&my_cpuset);
    for(i=0; i<configured_cpus && cpus_used<s->threads; i++){
        if( CPU_ISSET(i, &set) ){
            if( cpu_no >= (my_idx * s->threads) ){
                CPU_SET(i, &my_cpuset);
                cpus_used++;
            }
            cpu_no++;
        }
    }

finish:
    MPI_Allreduce(&error_loc, &error, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
    if( error ){
        if( 0 == grank ){
            fprintf(stderr, "ERROR (binding): %s\n", binding_err_msgs[error]);
        }
    }
}

void bind_worker(int tid, settings_t *s)
{
    int i, cpu_no = 0;

    if( !s->binding ){
        return;
    }

    for(i=0; i<configured_cpus; i++){
        if( CPU_ISSET(i, &my_cpuset) ){
            if( cpu_no == tid ){
                /* this core is mine! */
                cpu_set_t set;
                CPU_ZERO(&set);
                CPU_SET(i, &set);
                if( pthread_setaffinity_np(pthread_self(), sizeof(set), &set) ){
                    MPI_Abort(MPI_COMM_WORLD, 0);
                }
                break;
            }
            cpu_no++;
        }
    }

#ifdef DEBUG
    if( 0 == s->my_host_idx ){
        int rank;
        cpu_set_t set;

        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
        if( pthread_getaffinity_np(pthread_self(), sizeof(set), &set) ){
            MPI_Abort(MPI_COMM_WORLD, 0);
        }
        for(i=0; i<configured_cpus; i++){
            if( CPU_ISSET(i, &set) ){
                fprintf(stderr, "%d:%d: %d\n", rank, tid, i);
            }
        }
    }
#endif
}
