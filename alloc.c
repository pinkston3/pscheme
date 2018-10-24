#include "alloc.h"
#include "ptr_vector.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>


/*! Change to #define to output garbage-collector statistics. */
#undef GC_STATS

/*!
 * Change to #undef to cause the garbage collector to only run when it has to.
 * This dramatically improves performance.
 *
 * However, while testing GC, it's easiest if you try it all the time, so that
 * the number of objects being manipulated is small and easy to understand.
 */
#define ALWAYS_GC


/* Change to #define for other verbose output. */
#undef VERBOSE


void free_value(Value *v);
void free_lambda(Lambda *f);
void free_environment(Environment *env);

void mark_environment(Environment *env);
void mark_lambda(Lambda *lambda);
void mark_value(Value *v);
void mark_eval_stack(PtrStack *eval_stack);

void sweep_environments(void);
void sweep_lambdas(void);
void sweep_values(void);


/*========================================================*
 * TODO:  Declarations of your functions might go here... *
 *========================================================*/


/*!
 * A growable vector of pointers to all Value structs that are currently
 * allocated.
 */
static PtrVector allocated_values;


/*!
 * A growable vector of pointers to all Lambda structs that are currently
 * allocated.  Note that each Lambda struct will only have ONE Value struct that
 * points to it.
 */
static PtrVector allocated_lambdas;


/*!
 * A growable vector of pointers to all Environment structs that are currently
 * allocated.
 */
static PtrVector allocated_environments;


#ifndef ALWAYS_GC

/*! Starts at 1MB, and is doubled every time we can't stay within it. */
static long max_allocation_size = 1048576;

#endif


void init_alloc() {
    pv_init(&allocated_values);
    pv_init(&allocated_lambdas);
    pv_init(&allocated_environments);
}


/*!
 * This helper function prints some helpful details about the current allocation
 * status of the program.
 */
void print_alloc_stats(FILE *f) {
    /*
    fprintf(f, "Allocation statistics:\n");
    fprintf(f, "\tAllocated environments:  %u\n", allocated_environments.size);
    fprintf(f, "\tAllocated lambdas:  %u\n", allocated_lambdas.size);
    fprintf(f, "\tAllocated values:  %u\n", allocated_values.size);
    */

    fprintf(f, "%d vals \t%d lambdas \t%d envs\n", allocated_values.size,
        allocated_lambdas.size, allocated_environments.size);
}


/*!
 * This helper function returns the amount of memory currently being used by
 * garbage-collected objects.  It is NOT the total amount of memory being used
 * by the interpreter!
 */ 
long allocation_size() {
    long size = 0;
    
    size += sizeof(Value) * allocated_values.size;
    size += sizeof(Lambda) * allocated_lambdas.size;
    size += sizeof(Value) * allocated_environments.size;
    
    return size;
}


/*!
 * This function heap-allocates a new Value struct, initializes it to be empty,
 * and then records the struct's pointer in the allocated_values vector.
 */
Value * alloc_value(void) {
    Value *v = malloc(sizeof(Value));
    memset(v, 0, sizeof(Value));

    pv_add_elem(&allocated_values, v);

    return v;
}


/*!
 * This function frees a heap-allocated Value struct.  Since a Value struct can
 * represent several different kinds of values, the function looks at the
 * value's type tag to determine if additional memory needs to be freed for the
 * value.
 *
 * Note:  It is assumed that the value's pointer has already been removed from
 *        the allocated_values vector!  If this is not the case, serious errors
 *        will almost certainly occur.
 */
void free_value(Value *v) {
    assert(v != NULL);

    /*
     * If value refers to a lambda, we don't free it here!  Lambdas are freed
     * by the free_lambda() function, and that is called when cleaning up
     * unreachable objects.
     */

    if (v->type == T_String || v->type == T_Atom || v->type == T_Error)
        free(v->string_val);

    free(v);
}



/*!
 * This function heap-allocates a new Lambda struct, initializes it to be empty,
 * and then records the struct's pointer in the allocated_lambdas vector.
 */
Lambda * alloc_lambda(void) {
    Lambda *f = malloc(sizeof(Lambda));
    memset(f, 0, sizeof(Lambda));

    pv_add_elem(&allocated_lambdas, f);

    return f;
}


/*!
 * This function frees a heap-allocated Lambda struct.
 *
 * Note:  It is assumed that the lambda's pointer has already been removed from
 *        the allocated_labmdas vector!  If this is not the case, serious errors
 *        will almost certainly occur.
 */
void free_lambda(Lambda *f) {
    assert(f != NULL);

    /* Lambdas typically reference lists of Value objects for the argument-spec
     * and the body, but we don't need to free these here because they are
     * managed separately.
     */

    free(f);
}


/*!
 * This function heap-allocates a new Environment struct, initializes it to be
 * empty, and then records the struct's pointer in the allocated_environments
 * vector.
 */
Environment * alloc_environment(void) {
    Environment *env = malloc(sizeof(Environment));
    memset(env, 0, sizeof(Environment));

    pv_add_elem(&allocated_environments, env);

    return env;
}


/*!
 * This function frees a heap-allocated Environment struct.  The environment's
 * bindings are also freed since they are owned by the environment, but the
 * binding-values are not freed since they are externally managed.
 *
 * Note:  It is assumed that the environment's pointer has already been removed
 *        from the allocated_environments vector!  If this is not the case,
 *        serious errors will almost certainly occur.
 */
void free_environment(Environment *env) {
    int i;

    /* Free the bindings in the environment first. */
    for (i = 0; i < env->num_bindings; i++) {
        free(env->bindings[i].name);
        /* Don't free the value, since those are handled separately. */
    }
    free(env->bindings);

    /* Now free the environment object itself. */
    free(env);
}


/*!
 * This function performs the garbage collection for the Scheme interpreter.
 * It also contains code to track how many objects were collected on each run,
 * and also it can optionally be set to do GC when the total memory used grows
 * beyond a certain limit.
 */
void collect_garbage() {
    Environment *global_env;
    PtrStack *eval_stack;

#ifdef GC_STATS
    int vals_before, procs_before, envs_before;
    int vals_after, procs_after, envs_after;

    vals_before = allocated_values.size;
    procs_before = allocated_lambdas.size;
    envs_before = allocated_environments.size;
#endif

#ifndef ALWAYS_GC
    /* Don't perform garbage collection if we still have room to grow. */
    if (allocation_size() < max_allocation_size)
        return;
#endif

    /*==========================================================*
     * TODO:  Implement mark-and-sweep garbage collection here! *
     *                                                          *
     * Mark all objects referenceable from either the global    *
     * environment, or from the evaluation stack.  Then sweep   *
     * through all allocated objects, freeing unmarked objects. *
     *==========================================================*/

    global_env = get_global_environment();
    eval_stack = get_eval_stack();

    /*==========================================================*
     * Mark all objects referenceable from either the global    *
     * environment, or from the evaluation stack. Then sweep    *
     * through all allocated objects, freeing unmarked objects. *
     *==========================================================*/
    global_env = get_global_environment();
    eval_stack = get_eval_stack();

    /*
     * Mark all objects that are referenced from either the global environment
     * or the explicit stack.
     */
    mark_environment(global_env);
    mark_eval_stack(eval_stack);

    /*
     * Sweep through all objects and free each one that is no longer reachable.
     */
    sweep_values();
    sweep_lambdas();
    sweep_environments();

#ifndef ALWAYS_GC
    /* If we are still above the maximum allocation size, increase it. */
    if (allocation_size() > max_allocation_size) {
        max_allocation_size *= 2;

        printf("Increasing maximum allocation size to %ld bytes.\n",
            max_allocation_size);
    }
#endif
    
#ifdef GC_STATS
    vals_after = allocated_values.size;
    procs_after = allocated_lambdas.size;
    envs_after = allocated_environments.size;

    printf("GC Results:\n");
    printf("\tBefore: \t%d vals \t%d lambdas \t%d envs\n",
            vals_before, procs_before, envs_before);
    printf("\tAfter:  \t%d vals \t%d lambdas \t%d envs\n",
            vals_after, procs_after, envs_after);
    printf("\tChange: \t%d vals \t%d lambdas \t%d envs\n",
            vals_after - vals_before, procs_after - procs_before,
            envs_after - envs_before);
#endif
}


/*!
 * This function recursively marks a series of environments, starting at the
 * passed-in environment and continuing all the way up to the global
 * environment. The function also stops when it reaches an environment that has
 * already been marked.
 */
void mark_environment(Environment *env) {
    int i;

    assert(env != NULL); /* Mainly a sanity check. */

    while (env != NULL && !env->marked) {
        env->marked = 1;

        /* Recursively mark values in the environment. */
        for (i = 0; i < env->num_bindings; i++) {
#ifdef VERBOSE
            printf("Marking value bound to name \"%s\".\n",
                   env->bindings[i].name);
#endif
            mark_value(env->bindings[i].value);
        }

        /*
         * Mark this environment's parent-environment. We could recurse here,
         * but we will iterate instead to save stack space.
         */
        env = env->parent_env;
    }
}


/*!
 * This function marks all objects reachable from the evaluation stack. The
 * stack contains a number of evaluation contexts, each of which holds some
 * number of objects including the expression being evaluated, the environment
 * being used, and so forth. The main nuance here is that any of these values
 * may be NULL if the current evaluation context hasn't required a particular
 * value.
 */
void mark_eval_stack(PtrStack *eval_stack) {
    int ctx_idx, local_idx;
    EvaluationContext *ctx;

    assert(eval_stack != NULL);

    for (ctx_idx = 0; ctx_idx < eval_stack->size; ctx_idx++) {
        ctx = (EvaluationContext *) pv_get_elem(eval_stack, ctx_idx);

        /* Mark the values referenced by this stack entry. */

        if (ctx->current_env != NULL)
            mark_environment(ctx->current_env);

        if (ctx->expression != NULL)
            mark_value(ctx->expression);

        if (ctx->child_eval_result != NULL)
            mark_value(ctx->child_eval_result);

        for (local_idx = 0; local_idx < ctx->local_vals.size; local_idx++) {
            Value **pp_value =
                (Value **) pv_get_elem(&ctx->local_vals, local_idx);

            if (*pp_value != NULL) mark_value(*pp_value);
        }
    }
}


/*!
 * This function marks a Value object. If the Value object holds a simple
 * value then it is marked; if it holds a lambda or a cons-pair then the
 * contents of the object are recursively marked.
 *
 * There is one optimization here, which is that if the value is a cons-pair
 * then only the car is recursively marked; the cdr is iteratively marked, since
 * typically we will have lists of cons pairs.
 */
void mark_value(Value *v) {
    assert(v != NULL);

    if (v->marked) /* Already got here. */
        return;

    while (!v->marked) {
        v->marked = 1;

        if (v->type == T_Lambda) {
            /* Recursively mark the lambda expression. */
            mark_lambda(v->lambda_val);
        }
        else if (v->type == T_ConsPair) {
            /* Recursively mark the left side of the cons-pair. The typical
             * case will not be to have long chains of cons-pairs in the car.
             */
            mark_value(v->cons_val.p_car);

            /* Iteratively mark the right side of the cons-pair. The typical
             * case will be to have long chains of cons-pairs in the cdr.
             */
            v = v->cons_val.p_cdr;
        }
    }
}


/*!
 * This function marks a Lambda object. Since a Lambda uses chains of cons-pair
 * Values to represent its arguments and body, this function recursively uses
 * mark_value() to mark these components. Finally, the lambda's environment is
 * also marked.
 */
void mark_lambda(Lambda *lambda) {
    assert(lambda != NULL);

    if (lambda->marked) /* Already got here. */
        return;

    lambda->marked = 1;

    /* Native lambdas don't have argument-specifications or bodies. */
    if (!lambda->native_impl) {
        mark_value(lambda->arg_spec);
        mark_value(lambda->body);
    }

    mark_environment(lambda->parent_env);
}


/*!
 * This function iterates over all currently-allocated values, freeing unmarked
 * values, and also unmarking (but not freeing) marked values. At the end, the
 * vector of allocated values is compacted to remove all slots that were
 * occupied by values that we freed.
 */
void sweep_values(void) {
    unsigned int i;
    Value *v;

    for (i = 0; i < allocated_values.size; i++) {
        v = (Value *) pv_get_elem(&allocated_values, i);
        if (!v->marked) {
            free_value(v); /* unreachable. collect. */
            pv_set_elem(&allocated_values, i, NULL);
        }
        else {
            v->marked = 0; /* reachable. reset for next sweep. */
        }
    }

    pv_compact(&allocated_values);
}


/*!
 * This function iterates over all currently-allocated environments, freeing
 * unmarked environments, and also unmarking (but not freeing) marked
 * environments. At the end, the vector of allocated environments is compacted
 * to remove all slots that were occupied by environments that we freed.
 */
void sweep_environments(void) {
    unsigned int i;
    Environment *env;

    for (i = 0; i < allocated_environments.size; i++) {
        env = (Environment *) pv_get_elem(&allocated_environments, i);
        if (!env->marked) {
            free_environment(env); /* unreachable. collect. */
            pv_set_elem(&allocated_environments, i, NULL);
        }
        else {
            env->marked = 0; /* reachable. reset for next sweep. */
        }
    }

    pv_compact(&allocated_environments);
}


/*!
 * This function iterates over all currently-allocated lambdas, freeing unmarked
 * lambdas, and also unmarking (but not freeing) marked lambdas. At the end,
 * the vector of allocated lambdas is compacted to remove all slots that were
 * occupied by lambdas that we freed.
 */
void sweep_lambdas(void) {
    unsigned int i;
    Lambda *lambda;

    for (i = 0; i < allocated_lambdas.size; i++) {
        lambda = (Lambda *) pv_get_elem(&allocated_lambdas, i);
        if (!lambda->marked) {
            free_lambda(lambda); /* unreachable. collect. */
            pv_set_elem(&allocated_lambdas, i, NULL);
        }
        else {
            lambda->marked = 0; /* reachable. reset for next sweep. */
        }
    }

    pv_compact(&allocated_lambdas);
}

