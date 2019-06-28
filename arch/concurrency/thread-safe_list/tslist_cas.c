#include "common.h"
#include "tslist.h"
#include "arch.h"

tslist_elem_t **elem_pool = NULL;

tslist_t *tslist_create(int nthreads, int nelems)
{
    tslist_t *list = calloc(1, sizeof(*list));
    printf("list = %p\n", list);
    if( NULL == list ) {
        abort();
    }
    list->head = calloc(1, sizeof(*list->head));
    list->tail = list->head;
    printf("list->head = %p, list->tail = %p\n", list->head, list->tail);
    
    elem_pool = calloc(nthreads, sizeof(*elem_pool));
    int i;
    for(i=0; i<nthreads; i++){
        elem_pool[i] = calloc(nelems, sizeof(*elem_pool[0]));
    }
    return list;
}
void tslist_release(tslist_t *list)
{
    free(list->head);
    list->head = list->tail = NULL;
    free(list);
}


static void _append_to(tslist_t *list, tslist_elem_t *head)
{
    int count = 0;
    // Get approximate tail
    tslist_elem_t *last = list->tail;

    // Search for the exact tail and append to it
    while( 1 ) {
        while(last->next != NULL ){
            last = last->next;
            count++;
        }
        int64_t oldval = 0;
        if( CAS((int64_t*)&last->next, &oldval, (int64_t)head) ){
            atomic_isync();
            // If a pointer to the element was successfully added
            // set the new tail
            oldval = (int64_t)list->tail;
            if( count > nskip ) {
                // Only adjust the tale once in a while
                CAS((int64_t*)&list->tail, &oldval, (int64_t)head);
            }
            break;
        }
    }
}

__thread int pool_cnt = 0;
void tslist_append_batch(tslist_t *list, void **ptr, int count)
{
    int i;
    tslist_elem_t head_base = { 0 }, *head = &head_base;
    int tid = get_thread_id();

    for(i = 0; i < count; i++) {
        tslist_elem_t *elem = &elem_pool[tid][pool_cnt++];
        elem->ptr = ptr[i];
        elem->next = head->next;
        head->next = elem;
    }
    _append_to(list, head->next);
}


void tslist_append(tslist_t *list, void *ptr)
{
    tslist_append_batch(list, ptr, 1);
}

void tslist_append_done(tslist_t *list, int nthreads)
{
    /* This is a dummy function for fake list only */
}

tslist_elem_t *tslist_first(tslist_t *list)
{
    return list->head->next;
}

tslist_elem_t *tslist_next(tslist_elem_t *current)
{
    return current->next;
}

/*
Copyright 2019 Artem Y. Polyakov <artpol84@gmail.com>
Redistribution and use in source and binary forms, with or without modification,
are permitted provided that the following conditions are met:
1. Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
2. Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation and/or
other materials provided with the distribution.
3. Neither the name of the copyright holder nor the names of its contributors may
be used to endorse or promote products derived from this software without specific
prior written permission.
THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT,
INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE
OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED
OF THE POSSIBILITY OF SUCH DAMAGE.
*/
