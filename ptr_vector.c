#include "ptr_vector.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


/* Change to #define VERBOSE to turn on output. */
#undef VERBOSE


/*
 * These are helper functions, hence the declaration/definition only within this
 * module, and the "pvh_*" names.
 */

void pvh_reduce_capacity(PtrVector *pv);



/*! Initialize the data for a brand new pointer-vector. */
void pv_init(PtrVector *pv) {
    assert(pv != NULL);
    memset(pv, 0, sizeof(PtrVector));
}


/*!
 * Reset the state of the pointer-vector so that all memory allocation is freed.
 */
void pv_uninit(PtrVector *pv) {
    assert(pv != NULL);
    if (pv->elems != NULL)
        free(pv->elems);

    memset(pv, 0, sizeof(PtrVector));
}


/*!
 * Add an element to the end of a pointer-vector.  If the pointer-vector
 * already has space to accommodate the new pointer, it will simply add it to
 * the end of the vector.  Otherwise, the pointer-vector will increase its
 * memory allocation to store the new pointer.
 *
 * This function returns 1 if the add-element operation was successful, or 0 if
 * the operation failed for some reason (e.g. failure to allocate more memory).
 */
int pv_add_elem(PtrVector *pv, void *elem) {
    assert(pv != NULL);
    assert(elem != NULL);

    if (pv->size == pv->capacity) {
        /* Need to allocate more space for the pointer-vector. */

        unsigned int new_capacity;
        void **new_elems;

        new_capacity = 2 * pv->capacity;
        if (new_capacity == 0)
            new_capacity = 16;

        new_elems = (void **) realloc(pv->elems, new_capacity * sizeof(void *));

        if (new_elems == NULL)
            return 0;

        memset(new_elems + pv->capacity, 0,
               sizeof(void *) * (new_capacity - pv->capacity));

        pv->capacity = new_capacity;
        pv->elems = new_elems;
    }

    pv->elems[pv->size] = elem;
    pv->size++;

    return 1;
}


/*!
 * Retrieve an element at a specific index within the pointer-vector.  This
 * operation allows callers to access individual entries in the pointer-vector
 * in a random-access or sequential fashion.
 */
void * pv_get_elem(PtrVector *pv, unsigned int index) {
    assert(pv != NULL);
    assert(index < pv->size);

    return pv->elems[index];
}


/*!
 * Set an element at a specific index within the pointer-vector.  This operation
 * allows callers to modify individual entries in the pointer-vector in a
 * random-access or sequential fashion.
 */
void pv_set_elem(PtrVector *pv, unsigned int index, void *elem) {
    assert(pv != NULL);
    assert(index < pv->size);

    pv->elems[index] = elem;
}


/*!
 * This function compacts down the pointer-vector to take up the minimal space
 * necessary for the number of pointers it contains.  All elements set to NULL
 * are considered to be unused, and after this function runs, there should be no
 * NULL entries in the pointer-vector from indexes 0 to pv->size - 1.
 *
 * The implementation is very straightforward.  The function performs two
 * iterative operations each cycle through the pointer-vector:
 *
 * First, if the pointer-vector has NULL values at the end, the vector's size is
 * decreased to remove these elements.  Once this is finished, we know that the
 * element at pv->size - 1 is non-NULL.
 *
 * Second, we have another index that we start at the beginning of the
 * pointer-vector, and we increment this from the start all the way through to
 * the end, looking for NULL entries.  When we find a NULL entry, we move the
 * topmost (non-NULL) entry down into this spot, and then we go back to the
 * first part of the loop again.
 *
 * Repeatedly performing these two steps eventually results in a pointer-vector
 * with no NULL entries.
 *
 * Finally, the pointer-vector may have a much larger capacity than the current
 * number of elements really calls for, so the capacity is reduced until it is
 * appropriate for the current size of the pointer-vector.  Then, the memory in
 * use is reallocated to reduce the size of the vector.
 *
 * Once all of these steps are completed, the pointer-vector will occupy a
 * "minimal" amount of memory, given its current contents.
 */
void pv_compact(PtrVector *pv) {
    unsigned int empty_idx;
#ifdef VERBOSE
    unsigned int initial_size, initial_capacity;
#endif
    
    assert(pv != NULL);

#ifdef VERBOSE
    /* Store initial values so that we can print stats at the end. */
    initial_size = pv->size;
    initial_capacity = pv->capacity;
#endif

    empty_idx = 0;
    while (1) {
        /* Shrink down the size of the pointer-vector until the topmost element
         * is not NULL.
         */
        while (pv->size != 0 && pv->elems[pv->size - 1] == NULL)
            pv->size--;

        /* Find another empty slot in the pointer-vector. */
        while (empty_idx < pv->size && pv->elems[empty_idx] != NULL)
            empty_idx++;

        /* All done eliminating empty slots. */
        if (empty_idx == pv->size)
            break;

        /* Move topmost non-NULL slot down into the empty slot we have found. */
        pv->elems[empty_idx] = pv->elems[pv->size - 1];
        pv->elems[pv->size - 1] = NULL;
    }

    pvh_reduce_capacity(pv);

#ifdef VERBOSE
    fprintf(stderr, "pv_compact:  initial size = %u, final size = %u\n",
            initial_size, pv->size);
    fprintf(stderr, "pv_compact:  initial capacity = %u, final capacity = %u\n",
            initial_capacity, pv->capacity);
#endif
}


/*!
 * This helper function reduces the capacity of a PtrVector so that it only uses
 * as much capacity as necessary for the pointer-vector's current size.
 */
void pvh_reduce_capacity(PtrVector *pv) {
    unsigned int final_capacity;

    assert(pv != NULL);

    /* Shrink down the size of the pointer-array, since this will have also
     * grown very large.
     */
    final_capacity = pv->capacity;
    while (final_capacity / 2 > 16 && final_capacity / 2 > pv->size)
        final_capacity /= 2;

    if (final_capacity < pv->capacity) {
        /* It would be highly unusual for this realloc() to fail. */
        void **new_elems = (void **) realloc(pv->elems,
                                             final_capacity * sizeof(void *));
        if (new_elems != NULL) {
            pv->elems = new_elems;
            pv->capacity = final_capacity;
        }
    }
}




int ps_push_elem(PtrStack *ps, void *elem) {
    assert(ps != NULL);
    assert(elem != NULL);
    return pv_add_elem(ps, elem);
}


void * ps_pop_elem(PtrStack *ps) {
    void *elem;
    unsigned int tos_index;
    
    assert(ps != NULL);
    assert(ps->size > 0);
    
    /* Index of the top of the stack. */
    tos_index = ps->size - 1;
    
    elem = pv_get_elem(ps, tos_index);
    pv_set_elem(ps, tos_index, NULL);
    
    ps->size--;
    pvh_reduce_capacity(ps);

    return elem;
}


void * ps_peek_top(PtrStack *ps) {
    assert(ps != NULL);
    assert(ps->size > 0);
    return pv_get_elem(ps, ps->size - 1);
}


