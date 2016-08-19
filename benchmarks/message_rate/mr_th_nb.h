/*
 * Copyright (c) 2016      Mellanox Technologies Ltd. ALL RIGHTS RESERVED.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 *
 * $HEADER$
 */

#ifndef MR_TH_NB_H
#define MR_TH_NB_H

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

//#define DEBUG


typedef struct {
    void *(*worker)(void *);
    int threads;
    int win_size;
    int iterations;
    int warmup;
    int dup_comm;
    int msg_size;
    int binding;
    int want_thr_support;
    int intra_node;
    
    /* process mapping info */
    int my_partner;
    int i_am_sender;
    int my_host_idx;
    int my_rank_idx;
    int my_leader;

} settings_t;

void *(*worker)(void *);
void *worker_b(void *);
void *worker_nb(void *);

#endif