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


static void _append_to(tslist_t *list, tslist_elem_t *head,
                       tslist_elem_t *tail)
{
    tslist_elem_t *prev = (tslist_elem_t *)atomic_swap((uint64_t*)&list->tail, (uint64_t)tail);
    atomic_wmb();
    prev->next = head;
}

__thread int pool_cnt = 0;
void tslist_append_batch(tslist_t *list, void **ptr, int count)
{
    int i;
    tslist_elem_t head_base = { 0 }, *head = &head_base, *tail = head;
    int tid = get_thread_id();

    for(i = 0; i < count; i++) {
        tslist_elem_t *elem = &elem_pool[tid][pool_cnt++];
        elem->ptr = ptr[i];
        elem->next = NULL;
        tail->next = elem;
        tail = elem;
    }
    if( count ){
        _append_to(list, head->next, tail);
    }
}


void tslist_append(tslist_t *list, void *ptr)
{
    tslist_append_batch(list, ptr, 1);
}

int list_empty_cnt = 0;
int list_first_added_cnt = 0;
int list_extract_one_cnt = 0;
int list_extract_last_cnt = 0;

void tslist_dequeue(tslist_t *list, tslist_elem_t **_elem)
{
    *_elem = NULL;
    if( list->head == list->tail ){
        // the list is empty
        list_empty_cnt++;
        return;
    }

    if(list->head->next == NULL ){
        // Someone is adding a new element, but it is not yet ready
        list_first_added_cnt++;
        return;
    }

    tslist_elem_t *elem = list->head->next;
    if( elem->next ) {
        /* We have more than one elements in the list
         * it is safe to dequeue this elemen as only one thread
         * is allowed to dequeue
         */
        atomic_isync();
        list->head->next = elem->next;
        *_elem = elem;
        /* Terminate element */
        elem->next = NULL;
        /* No need to release in this test suite */
        list_extract_one_cnt++;
        return;
    }

    tslist_elem_t *iter;
    /* requeue the dummy element to make sure that no one is adding currently */
    list->head->next = NULL;
    _append_to(list, list->head, list->head);
    iter = elem;
    while(iter->next != list->head) {
        atomic_isync();
        if((volatile void*)iter->next) {
            iter = iter->next;
        }
    }
    /* Terminate the extracted chain */
    iter->next = NULL;
    *_elem = elem;
    list_extract_last_cnt++;
    printf("!!!\n");
}

void tslist_append_done(tslist_t *list, int nthreads)
{
    /* This is a dummy function for fake list only */
    printf("list_empty_cnt = %d\n", list_empty_cnt);
    printf("list_first_added_cnt =%d\n", list_first_added_cnt);
    printf("list_extract_one_cnt = %d\n", list_extract_one_cnt);
    printf("list_extract_last_cnt = %d\n", list_extract_last_cnt);
}

tslist_elem_t *tslist_first(tslist_t *list)
{
    return list->head->next;
}

tslist_elem_t *tslist_next(tslist_elem_t *current)
{
    return current->next;
}
