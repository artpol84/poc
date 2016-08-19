#ifndef THREAD_SYNC_H
#define THREAD_SYNC_H

#include <stdio.h>
#include "mr_th_nb.h"

#define WMB() \
{ asm volatile ("" : : : "memory"); }


void sync_init(int nthreads);
void sync_force_single();
void sync_worker(int tid);
void sync_master();
void setup_binding(MPI_Comm comm, settings_t *s);
void bind_worker(int tid, settings_t *s);


#endif