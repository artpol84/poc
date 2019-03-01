#include "common.h"
#include "tslist.h"
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

void tslist_append(tslist_t *list, void *ptr)
{
    // Get approximate tail
    tslist_elem_t *last = list->tail;
    // Create a new element
    tslist_elem_t *elem = calloc(1, sizeof(*elem));
    elem->ptr = ptr;
    elem->next = NULL;
    // store barrier

    // Search for the exact tail and append to it
    while( 1 ) {
        while(last->next != NULL ){
            last = last->next;
        }
        int64_t oldval = (int64_t)last->next;
        if( CAS((int64_t*)&last->next, &oldval, (int64_t)elem) ){
            // If a pointer to the element was successfully added
            // set the new tail
            oldval = (int64_t)list->tail;
            CAS((int64_t*)&list->tail, &oldval, (int64_t)elem);
            break;
        }
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

