#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "evaluator.h"
#include "alloc.h"
#include "native_lambdas.h"
#include "special_forms.h"


#undef VERBOSE_EVAL


/*! This is the global environment used for evaluation of Scheme programs. */
static Environment *global_env = NULL;

/*! This is the explicit stack used for evaluation of Scheme programs. */
static PtrStack evaluation_stack = PTR_VECTOR_STATIC_INIT;


Value * bind_arguments(Environment *child_env, Lambda *lambda, Value *operands);


/*!
 * This struct is used for representing Scheme functions with native
 * implementations.  The implementation is provided as a function-pointer with
 * the signature NativeLambda.
 */
typedef struct NativeLambdaBinding {
    char *name;
    NativeLambda func;
} NativeLambdaBinding;


/*!
 * This is a collection of the native built-in functions provided by this Scheme
 * implementation.  At this point it's pretty limited, but it would be easy to
 * extend.
 *
 * The array of native lambdas must end with a NativeLambdaBinding struct that
 * contains NULL for both elements.  This makes it very easy to iterate over the
 * array's contents.
 */
static NativeLambdaBinding native_lambdas[] = {
    { "=" , scheme_numeric_equals        },
    { "<" , scheme_numeric_less_than     },
    { ">" , scheme_numeric_greater_than  },
    { "<=", scheme_numeric_less_equal    },
    { ">=", scheme_numeric_greater_equal },

    { "eq?"   , scheme_eq    },
    { "equal?", scheme_equal },

    { "boolean?"  , scheme_is_boolean   },
    { "number?"   , scheme_is_number    },
    { "pair?"     , scheme_is_pair      },
    { "procedure?", scheme_is_procedure },
    { "string?"   , scheme_is_string    },
    { "symbol?"   , scheme_is_symbol    },

    { "+", scheme_add },
    { "-", scheme_sub },
    { "*", scheme_mul },
    { "/", scheme_div },

    /* Functions for cons pairs and lists. */
    { "cons"  , scheme_cons   },
    { "car"   , scheme_car    },
    { "cdr"   , scheme_cdr    },
    { "list"  , scheme_list   },
    { "length", scheme_length },

    /* In-place mutation functions. */
    { "set-car!", scheme_set_car },
    { "set-cdr!", scheme_set_cdr },

    /* Utility functions. */
    { "display"  , scheme_display   },
    { "error"    , scheme_error     },
    { "srandom"  , scheme_srandom   },
    { "random"   , scheme_random    },
    { "time"     , scheme_time      },
    { "sqrt"     , scheme_sqrt      },
    { "eval-file", scheme_eval_file },

    /* Terminator. */
    { NULL, NULL }
};




/*!
 * Creates and initializes a new environment struct, having the specified parent
 * environment.  This function supports a NULL input-value for the parent
 * environment, but really, only the global environment should have a NULL
 * parent-environment.
 */
Environment * make_environment(Environment *parent_env) {
    Environment *env = alloc_environment();
    env->parent_env = parent_env;

    return env;
}


/*!
 * Creates and initializes a new environment struct for the global environment.
 * The global environment is the root of all other environments, and has a
 * couple of unique characteristics.  First, it has no parent environment
 * (obvious, since it's the root of all environments).  Second, all built-in
 * functions are bound into this environment.
 */
Environment * init_global_environment(void) {
    NativeLambdaBinding *binding;

    assert(global_env == NULL);
    global_env = make_environment(NULL);

    binding = native_lambdas;
    while (binding->name != NULL) {
        create_binding(global_env, binding->name,
                       make_native_lambda(global_env, binding->func));

        binding++;
    }

    return global_env;
}


Environment * get_global_environment(void) {
    return global_env;
}


/*!
 * Attempts to create a new binding in the specified environment, and returns a
 * status value indicating success or failure.  Success is indicated by a result
 * of 1, and failure is indicated by a 0 result.  If the specified name already
 * appears in this environment, then the previous binding is replaced.
 *
 * The only way this function will fail is if the function can't allocate the
 * necessary memory.
 */
int create_binding(Environment *env, char *name, Value *v) {
    int i;

    assert(env != NULL);
    assert(name != NULL);
    assert(v != NULL);

    for (i = 0; i < env->num_bindings; i++) {
        if (strcmp(env->bindings[i].name, name) == 0) {
            env->bindings[i].value = v;
            return 1;
        }
    }

    if (env->num_bindings == env->capacity) {
        Binding *new_bindings;
        int new_capacity;

        if (env->capacity == 0)
            new_capacity = 4;
        else
            new_capacity = env->capacity * 2;

        new_bindings = realloc(env->bindings, new_capacity * sizeof(Binding));
        if (!new_bindings)
            return 0;      /* epic fail. */

        env->capacity = new_capacity;
        env->bindings = new_bindings;
    }

    assert(env->num_bindings < env->capacity);

    i = env->num_bindings;
    env->bindings[i].name = strdup(name);
    env->bindings[i].value = v;
    env->num_bindings++;

    return 1;
}


/*!
 * Update an existing binding with a new value, returning success (0) or failure
 * (nonzero) depending on whether the operation succeeds.  The important detail
 * of this operation is that no NEW binding is created; an EXISTING binding is
 * simply updated.  If the current environment doesn't have a binding with the
 * specified name, this function switches to the environment's parent and looks
 * again.  This continues all the way up to the root environment, and if the
 * root doesn't contain a binding with the specified name, then the function
 * returns a failure value (nonzero).
 */
int update_binding(Environment *env, char *name, Value *v) {
    int i;

    assert(env != NULL);
    assert(name != NULL);
    assert(v != NULL);

    do {
        /* Look for a binding with the specified name.  If we find it, update
         * the value and return success.  Otherwise, we'll move to the parent
         * of this environment.
         */
        for (i = 0; i < env->num_bindings; i++) {
            if (strcmp(env->bindings[i].name, name) == 0) {
                env->bindings[i].value = v;
                return 1;
            }
        }

        /*
         * Couldn't find binding in this environment.
         * Go to environment's parent.
         */
        env = env->parent_env;
    }
    while (env != NULL);

    return 0;
}


/*!
 * Given a name and a starting environment, this function resolves the name to
 * a value using the bindings in the environment.  If the environment doesn't
 * contain any bindings with the specified name, the function moves to the
 * environment's parent environment and repeats the search.  This continues all
 * the way up to the root environment.
 *
 * If the name cannot be resolved to a value then this function returns NULL.
 */
Value * resolve_binding(Environment *env, char *name) {
    int i;

    assert(env != NULL);
    assert(name != NULL);

    /* Starting with the original environment, search for the specified name. */
    do {
        for (i = 0; i < env->num_bindings; i++) {
            if (strcmp(env->bindings[i].name, name) == 0) {
                Value *v = env->bindings[i].value;

#ifdef VERBOSE_EVAL
                printf("\tName \"%s\" is bound to:  ", name);
                print_value(stdout, v);
                printf("\n");
#endif

                return v;
            }
        }

        /* Couldn't find binding in this environment.
         * Go to the environment's parent.
         */
        env = env->parent_env;
    }
    while (env != NULL);

    return NULL;
}



Value * bind_names_values(Environment *env, Value *names, Value *values) {

    assert(env != NULL);
    assert(names != NULL);
    assert(values != NULL);

    /* Bind each value under its specified name. */
    while (is_cons_pair(names)) {
        Value *argname = get_car(names);

        if (is_nil(values))
            return make_error("not enough values to complete bindings!");

        assert(is_cons_pair(values));

        create_binding(env, argname->string_val, get_car(values));
    
        names = get_cdr(names);
        values = get_cdr(values);
    }

    assert(is_nil(names));
    if (!is_nil(values))
        return make_error("too many values for the specified bindings!");

    return NULL;
}



PtrStack * get_eval_stack(void) {
    return &evaluation_stack;
}


/*!
 * \return The new evaluation-context, or NULL if one could not be created.
 */
EvaluationContext * push_new_evalctx(Environment *env, Value *expr) {
    EvaluationContext *ctx;

    ctx = (EvaluationContext *) malloc(sizeof(EvaluationContext));
    if (ctx != NULL) {
        memset(ctx, 0, sizeof(EvaluationContext));
        ps_push_elem(&evaluation_stack, ctx);
    }
    
    /* Set up the new values for the environment. */
    ctx->current_env = env;
    ctx->expression = expr;
    
    return ctx;
}


EvaluationContext * get_current_evalctx(void) {
    EvaluationContext *ctx = NULL;

    if (evaluation_stack.size > 0)
        ctx = (EvaluationContext *) ps_peek_top(&evaluation_stack);

    return ctx;
}


EvaluationContext * reset_current_evalctx(Environment *env, Value *expr) {
    EvaluationContext *ctx = get_current_evalctx();

    /* Reset the context! */
    pv_uninit(&ctx->local_vals);
    memset(ctx, 0, sizeof(EvaluationContext));
    
    /* Set up the new values for the environment. */
    ctx->current_env = env;
    ctx->expression = expr;
    
    /* For convenience, return the context pointer. */
    return ctx;
}


void evalctx_register(Value **v) {
    EvaluationContext *ctx;
    
    assert(v != NULL);
    ctx = get_current_evalctx();
    pv_add_elem(&ctx->local_vals, v);
    
    *v = NULL;
}


/*!
 * This function pops and cleans up the current evaluation context.  The
 * function only releases memory used by the eval-context itself; it doesn't
 * free any Value or Environment objects referenced by the context, since that
 * will be collected by the garbage collector.
 *
 * This function will have an assertion-failure if the evaluation stack is
 * empty when this function is called.
 */
void pop_evalctx(Value *result) {
    EvaluationContext *ctx, *parent_ctx;

    ctx = (EvaluationContext *) ps_pop_elem(&evaluation_stack);
    
    /* Clean up the context. */
    pv_uninit(&ctx->local_vals);
    free(ctx);
    
    /* Store the result of the evaluation into the parent context's "child
     * result" slot.
     */
    parent_ctx = (EvaluationContext *) ps_peek_top(&evaluation_stack);
    parent_ctx->child_eval_result = result;
}



Value * evaluate(Environment *env, Value *expr) {

    EvaluationContext *ctx;
    Value *temp, *result;

    Value *operator;
    Value *operand_val, *operand_cons;
    Value *operands, *operands_end, *nil_value;
    int num_operands;
    
    /* Set up a new evaluation context and record our local variables, so that
     * the garbage-collector can see any temporary values we use.
     */
    ctx = push_new_evalctx(env, expr);
    evalctx_register(&temp);
    evalctx_register(&result);
    evalctx_register(&operator);
    evalctx_register(&operand_val);
    evalctx_register(&operand_cons);
    evalctx_register(&operands);
    evalctx_register(&operands_end);
    evalctx_register(&nil_value);
    
#ifdef VERBOSE_EVAL
    printf("\nEvaluating expression:  ");
    print_value(stdout, expr);
    printf("\n");
#endif

    /* If this is a special form, evaluate it.  Otherwise, this function will
     * simply pass the input through to the result.
     */
    result = eval_special_form(env, expr);
    if (result != expr)
        goto Done;    /* It was a special form. */

    /*
     * If the input is an atom, we need to resolve it to a value, using the
     * current environment.
     */

    if (is_atom(expr)) {
        /* Treat the atom as a name - resolve it to a value. */
        result = resolve_binding(env, expr->string_val);
        if (result == NULL) {
            result = make_error("couldn't resolve name \"%s\" to a value!",
                expr->string_val);
        }

        goto Done;
    }

    /*
     * If the input isn't an atom and isn't a cons-pair, then assume it's a
     * value that doesn't need evaluating, and just return it.
     */

    if (!is_cons_pair(expr)) {
        result = expr;
        goto Done;
    }

    /*
     * Evaluate operator into a lambda expression.
     */

    temp = get_car(expr);

    operator = evaluate(env, temp);
    if (is_error(operator)) {
        result = operator;
        goto Done;
    }
    if (!is_lambda(operator)) {
        result = make_error("operator is not a valid lambda expression");
        goto Done;
    }

#ifdef VERBOSE_EVAL
    printf("Operator:  ");
    print_value(stdout, operator);
    printf("\n");
#endif

    /*
     * Evaluate each operand into a value, and build a list up of the values.
     */

#ifdef VERBOSE_EVAL
    printf("Starting evaluation of operands.\n");
#endif

    num_operands = 0;
    operands_end = NULL;
    operands = nil_value = make_nil();

    temp = get_cdr(expr);
    while (is_cons_pair(temp)) {
        Value *raw_operand;
        
        num_operands++;

        /* This is the raw unevaluated value. */
        raw_operand = get_car(temp);

        /* Evaluate the raw input into a value. */
        
        operand_val = evaluate(env, raw_operand);
        if (is_error(operand_val)) {
            result = operand_val;
            goto Done;
        }

        operand_cons = make_cons(operand_val, nil_value);
        if (operands_end != NULL)
            set_cdr(operands_end, operand_cons);
        else
            operands = operand_cons;

        operands_end = operand_cons;

        temp = get_cdr(temp);
    }

    /*
     * Apply the operator to the operands, to generate a result.
     */

    if (operator->lambda_val->native_impl) {
        /* Native lambdas don't need an environment created for them.  Rather,
         * we just pass the list of arguments to the native function, and it
         * processes the arguments as needed.
         */
        result = operator->lambda_val->func(num_operands, operands);
    }
    else {
        /* These don't need registered on the explicit stack.  (I hope.) */
        Environment *child_env;
        Value *body_iter;

        /* It's an interpreted lambda.  Create a child environment, then
         * populate it with values based on the lambda's argument-specification
         * and the input operands.
         */
        child_env = make_environment(operator->lambda_val->parent_env);
        temp = bind_arguments(child_env, operator->lambda_val, operands);
        if (is_error(temp)) {
            result = temp;
            goto Done;
        }

        /* Evaluate each expression in the lambda, using the child environment.
         * The result of the last expression is the result of the lambda.
         */
        body_iter = operator->lambda_val->body;
        do {
            result = evaluate(child_env, get_car(body_iter));
            body_iter = get_cdr(body_iter);
        }
        while (!is_nil(body_iter));
    }

Done:
    
#ifdef VERBOSE_EVAL
    printf("Result:  ");
    print_value(stdout, result);
    printf("\n\n");
#endif

    /* Record the result and then perform garbage-collection. */
    pop_evalctx(result);
    collect_garbage();

    return result;
}


/*!
 * This helper function takes an interpreted lambda expression, a list of
 * evaluated operands for the lambda, and the environment that the lambda will
 * be run in, and binds each operand into the environment with the argument name
 * specified in the lambda's argument list.
 *
 * The function returns NULL on success, or a Value* of type T_Error if
 * something horrible happens along the way.  For example, the function may
 * receive too many or too few arguments, or the interpreter may not have enough
 * memory to construct a specific binding.
 */
Value * bind_arguments(Environment *child_env, Lambda *lambda, Value *operands) {
    Value *argname_iter, *argval_iter;

    assert(child_env != NULL);
    assert(lambda != NULL);
    assert(!lambda->native_impl);
    assert(operands != NULL);

    /* Populate the child environment with values based on the lambda's
     * argument-specification, and the input operands.
     */
    argname_iter = lambda->arg_spec;
    argval_iter = operands;
    if (is_atom(argname_iter)) {
        /* Entire operand list gets bound under a single name. */
        create_binding(child_env, argname_iter->string_val, operands);
    }
    else {
        /* Bind each operand under its specified name.
         *
         * TODO:  Find a way to use bind_names_values(), one day...
         */
        assert(is_cons_pair(argname_iter));
        do {
            Value *argname = get_car(argname_iter);

            if (is_nil(argval_iter))
                return make_error("not enough arguments for lambda!");

            assert(is_cons_pair(operands));

            create_binding(child_env, argname->string_val,
                           get_car(argval_iter));

            argname_iter = get_cdr(argname_iter);
            argval_iter = get_cdr(argval_iter);
        }
        while (is_cons_pair(argname_iter));

        if (is_nil(argname_iter) && !is_nil(argval_iter))
            return make_error("too many arguments for lambda!");

        /* If argname_iter is an atom then the argument-list was an improper
         * list, and the remainder of the operands get bound under this name.
         */
        if (is_atom(argname_iter)) {
            create_binding(child_env, argname_iter->string_val, argval_iter);
        }
        else {
            /* The lambda special-form should have constructed the argument
             * specification properly, so this assertion should never fail.
             */
            assert(is_nil(argname_iter));
        }
    }

    /* NULL just means "success" here.  The only other thing this function might
     * return is a Value of type T_Error, if something awful happened...
     */
    return NULL;
}




