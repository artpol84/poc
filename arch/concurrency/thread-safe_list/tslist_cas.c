#include "common.h"
#include "tslist_cas.h"
#include "x86.h"



tslist_t *tslist_create()
{
    tslist_t *list = calloc(1, sizeof(*list));
    if( NULL == list ) {
        abort();
    }
    list->head = calloc(1, sizeof(*list->head));
    list->tail = list->head;

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
            // If a pointer to the element was successfully added
            // set the new tail
            oldval = (int64_t)list->tail;
            if( count > nskip ) {
                // Only adjust te tale once in a while
                CAS((int64_t*)&list->tail, &oldval, (int64_t)head);
            }
            break;
        }
    }
}

void tslist_append_batch(tslist_t *list, void **ptr, int count)
{
    int i;
    tslist_elem_t *head = calloc(1,sizeof(*head));

    for(i = 0; i < count; i++) {
        tslist_elem_t *elem = calloc(1, sizeof(*elem));
        elem->ptr = ptr[i];
        elem->next = head->next;
        head->next = elem;
    }
    _append_to(list, head->next);
    free(head);
}


void tslist_append(tslist_t *list, void *ptr)
{
    tslist_append_batch(list, ptr, 1);
}


tslist_elem_t *tslist_first(tslist_t *list)
{
    return list->head->next;
}

tslist_elem_t *tslist_next(tslist_elem_t *current)
{
    return current->next;
}

