#ifndef PTR_VECTOR_H
#define PTR_VECTOR_H



/*!
 * A growable vector whose elements are void-pointers.
 */
typedef struct PtrVector {
    /*! Number of elements the pointer-vector *could* hold. */
    unsigned int capacity;

    /*! Number of elements the pointer-vector currently holds. */
    unsigned int size;

    /*! The array of elements in the pointer-vector. */
    void **elems;
} PtrVector;


/*!
 * In cases where a pointer-vector or pointer-stack is statically declared,
 * this symbol is defined to the struct-initialization code to initialize the
 * pointer-vector's values properly.
 */
#define PTR_VECTOR_STATIC_INIT { 0, 0, NULL }


void pv_init(PtrVector *pv);
void pv_uninit(PtrVector *pv);

int pv_add_elem(PtrVector *pv, void *elem);
void * pv_get_elem(PtrVector *pv, unsigned int index);
void pv_set_elem(PtrVector *pv, unsigned int index, void *elem);
void pv_compact(PtrVector *pv);


/*!
 * A stack whose elements are void-pointers.  Since we use the PtrVector
 * implementation, we'll go ahead and give this a nickname too.
 */
typedef PtrVector PtrStack;

int ps_push_elem(PtrStack *ps, void *elem);
void * ps_pop_elem(PtrStack *ps);
void * ps_peek_top(PtrStack *ps);



#endif /* PTR_VECTOR_H */

