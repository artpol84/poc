#ifndef __USE_GNU
#define __USE_GNU
#endif
#define _GNU_SOURCE

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

uint64_t rdtsc() {
    uint64_t ts;
    asm volatile ( 
        "rdtsc\n\t"    // Returns the time in EDX:EAX.
        "shl $32, %%rdx\n\t"  // Shift the upper bits left.
        "or %%rdx, %0"        // 'Or' in the lower bits.
        : "=a" (ts)
        : 
        : "rdx");
    return ts;
}

void spinlock_artpol(pthread_spinlock_t *l)
{
    asm volatile (
        "artpol_acquire:\n"
        "    lock decl (%[lock])\n"
        "    jne    artpol_sleep\n"
        "    jmp artpol_exit\n"
        "artpol_sleep:\n"
        "    pause\n"
        "    cmpl   $0x0, (%[lock])\n"
        "    jg     artpol_acquire\n"
        "    jmp    artpol_sleep\n"
        "artpol_exit:\n"
        :
        : [lock] "r" (l)
        : "memory" );
}

uint64_t spinlock_prof_cnt(pthread_spinlock_t *l)
{
    uint64_t wait_count = 4294967295;
//    uint64_t wait_count = 2294967295;
    asm volatile (
        "slk_acquire_%=:\n"
        "    lock decl (%[lock])\n"
        "    jne slk_sleep_%=\n"
        "    jmp slk_exit_%=\n"
        "slk_sleep_%=:\n"
        "    pause\n"
        "    incq %%rax\n"
        "    cmpl   $0x0, (%[lock])\n"
        "    jg     slk_acquire_%=\n"
        "    jmp    slk_sleep_%=\n"
        "slk_exit_%=:\n"
        : "=a" (wait_count)
        : [lock] "r" (l), "a" (wait_count)
        : "memory" );
    return wait_count;
}

uint64_t spinlock_prof_cycles(pthread_spinlock_t *l)
{
    uint64_t ts1, ts2, ret = 0, cntr = 0;
    uint64_t ts1e, ts2e;

    ts1e = rdtsc();
    asm volatile (
        // Use rdx as a flag, if it is 0 - timestamp wasn't taken
        "    xor %%r10, %%r10\n"
        "    xor %%rax, %%rax\n"
        "    lock decl (%[lock])\n"
        "    je slk_exit_%=\n"
        // Get the spin start timestamp
        "    rdtsc\n"
        "    shl $32, %%rdx\n"
        "    or %%rax, %%rdx\n"
        "    mov %%rdx, %%r10\n"
        "    xor %%rax, %%rax\n"
        "    jmp slk_sleep_%=\n"
        "slk_acquire_%=:\n"
        "    lock decl (%[lock])\n"
        "    jne slk_sleep_%=\n"
        "    jmp slk_exit_%=\n"
        "slk_sleep_%=:\n"
        "    pause\n"
        "    incq %%rax\n"
        "    cmpl   $0x0, (%[lock])\n"
        "    jg     slk_acquire_%=\n"
        "    jmp    slk_sleep_%=\n"
        "slk_exit_%=:\n"
        // Get the spin stop timestamp
        "    mov %%rax, (%[cntr])\n"
        "    xor %%rdx, %%rdx\n"
        "    xor %%rax, %%rax\n"
        "    rdtsc\n"
        "    shl $32, %%rdx\n"
        "    or %%rax, %%rdx\n"
        "    mov %%rdx, (%[ts2])\n"
        "    mov %%r10, (%[ts1])\n"
        :
        : [lock] "r" (l), [ts1] "r" (&ts1), [ts2] "r" (&ts2), [cntr] "r" (&cntr)
        : "memory", "rax", "rdx", "r10", "rcx");
    ts2e = rdtsc();

    printf("ts1 = %lu, ts2 = %lu, cntr = %lu\n", ts1, ts2, cntr);
    printf("ts1e = %lu, ts2e = %lu, diff=%lu\n", ts1e, ts2e, ts2e - ts1e);
    if (ts1 != 0) {
        ret = ts2 - ts1;
    }
    return ret;
}

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
    printf("Wait cycles = %lu\n",
        spinlock_prof_cycles(&lock));
}

static void lock_lock1(int id)
{
    printf("Wait cycles = %lu\n",
        spinlock_prof_cycles(&lock));
}

static void lock_unlock(int id)
{
    pthread_spin_unlock(&lock);
}

pthread_barrier_t tbarrier;



void *worker(void *_id)
{
    int i, tid = *((int*)_id);

    if( tid == 0 ){
        lock_lock(tid);
    }

    pthread_barrier_wait(&tbarrier);

    if( tid == 1 ){
        lock_lock1(tid);
    } else {

        int delay = 1;
//        while( delay )
        {
            sleep(1);
        }
    }
    lock_unlock(tid);
}

int main(int argc, char **argv)
{
    int i;

    pthread_barrier_init(&tbarrier, NULL, 2);
    int tids[2];
    pthread_t id[2];

    lock_init();

    /* setup and create threads */
    for (i=0; i<2; i++) {
        tids[i] = i;
        pthread_create(&id[i], NULL, worker, (void *)&tids[i]);
    }

    for (i=0; i<2; i++) {
        pthread_join(id[i], NULL);
    }
}
