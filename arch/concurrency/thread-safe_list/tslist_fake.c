#include "common.h"
#include "tslist.h"
#include "x86.h"

tslist_t **tl_lists = NULL;
tslist_elem_t **elem_pool = NULL;

static tslist_t *_init_tslist()
{
    tslist_t *list = calloc(1, sizeof(*list));
    if( NULL == list ) {
        abort();
    }
    list->head = calloc(1, sizeof(*list->head));
    list->tail = list->head;

    return list;
}

tslist_t *tslist_create(int nthreads, int nelems)
{
    tl_lists = calloc(nthreads, sizeof(*tl_lists));
    int i;
    for(i = 0; i < nthreads; i++) {
        tl_lists[i] = _init_tslist();
    }

    elem_pool = calloc(nthreads, sizeof(*elem_pool));
    for(i=0; i<nthreads; i++){
        elem_pool[i] = calloc(nelems, sizeof(*elem_pool[0]));
    }

    return _init_tslist();
}
void tslist_release(tslist_t *list)
{
    free(list->head);
    list->head = list->tail = NULL;
    free(list);
}


__thread int pool_cnt = 0;
void tslist_append_batch(tslist_t *list, void **ptr, int count)
{
    int i;
    int tid = get_thread_id();

    for(i = 0; i < count; i++) {
        tslist_elem_t *elem = &elem_pool[tid][pool_cnt++];
        elem->ptr = ptr[i];
        elem->next = NULL;
        tl_lists[tid]->tail->next = elem;
        tl_lists[tid]->tail = elem;
    }
}

void tslist_append(tslist_t *list, void *ptr)
{
    tslist_append_batch(list, ptr, 1);
}

void tslist_append_done(tslist_t *list, int nthreads)
{
    int i;
    for(i = 0; i < nthreads; i++) {
        list->tail->next = tl_lists[i]->head->next;
        list->tail = tl_lists[i]->tail;
    }
}

tslist_elem_t *tslist_first(tslist_t *list)
{
    return list->head->next;
}

tslist_elem_t *tslist_next(tslist_elem_t *current)
{
    return current->next;
}

