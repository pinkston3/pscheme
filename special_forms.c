#include "special_forms.h"
#include "values.h"
#include "evaluator.h"

#include <assert.h>
#include <string.h>


/*
 * Each of these functions is used to evaluate a specific special form.  They
 * all have the signature of the SpecialFormEvaluator typedef.
 */

Value * eval_begin(Environment *env, Value *expr);
Value * eval_let(Environment *env, Value *expr);
Value * eval_if(Environment *env, Value *expr);
Value * eval_and(Environment *env, Value *expr);
Value * eval_or(Environment *env, Value *expr);
Value * eval_cond(Environment *env, Value *expr);
Value * eval_define(Environment *env, Value *expr);
Value * eval_lambda(Environment *env, Value *expr);
Value * eval_quote(Environment *env, Value *expr);
Value * eval_set_bang(Environment *env, Value *expr);

/* Helper function for eval_define. */
Value * eval_sugared_define(Environment *env, Value *expr);


/*!
 * This typedef defines a function-pointer type that is used for evaluating
 * special forms.  All the special-form evaluation functions conform to this
 * signature.
 */
typedef Value * (*SpecialFormEvaluator)(Environment *env, Value *expr);


/*!
 * This struct is used to keep track of which atom-name goes with each
 * special-form evaluation function.  It is used in the special_forms variable
 * below, which is used during expression evaluation to identify if an
 * expression is in fact a special form.
 */
typedef struct SpecialForm {
    char *name;
    SpecialFormEvaluator func;
} SpecialForm;


/*!
 * This array of SpecialForm structs simply lists all the special forms we
 * recognize, along with the evaluation function used for each special form.
 * The array MUST end with a SpecialForm with NULL for both values.
 *
 * If we want to be really spiffy, we can change this into a pointer-vector, and
 * provide support for adding new special forms to the language.  But right now
 * we aren't spiffy at all.
 */
SpecialForm special_forms[] = {
    { "begin" , eval_begin    },
    { "let"   , eval_let      },
    { "if"    , eval_if       },
    { "and"   , eval_and      },
    { "or"    , eval_or       },
    { "cond"  , eval_cond     },
    { "define", eval_define   },
    { "lambda", eval_lambda   },
    { "quote" , eval_quote    },
    { "set!"  , eval_set_bang },
    
    { NULL, NULL }  /* Terminator. */
};



/*!
 * This function determines if the passed-in expression is a special form that
 * it recognizes, and if so, the special form is evaluated according to the
 * rules for that form.
 *
 * If the passed-in expression is NOT a special form then the original
 * expression is returned completely unmodified.  Thus, the caller can tell if
 * an expression was a special form, if the result-pointer is different from the
 * argument-pointer.
 */
Value * eval_special_form(Environment *env, Value *expr) {
    Value *car;
    char *string_val;
    int i;
    Value *result = expr;

    if (!is_cons_pair(expr))
        goto Done;

    car = expr->cons_val.p_car;
    if (!is_atom(car))
        goto Done;

    /*
     * If we recognize the name, evaluate it as a special form.  If we don't see
     * the name in our list though, just leave the result unchanged.
     */

    string_val = car->string_val;
    for (i = 0; special_forms[i].name != NULL; i++) {
        if (strcmp(string_val, special_forms[i].name) == 0)
            result = special_forms[i].func(env, expr);
    }

Done:
    return result;
}


/*!
 * This function handles the begin special form:  (begin expr1 expr2 ...)
 *
 * This special form simply groups multiple expressions and evaluates them in
 * sequence.  The result of the evaluation is simply the last expression's
 * result.
 */
Value * eval_begin(Environment *env, Value *expr) {
    Value *subexpr;
    Value *result = NULL;

    expr = get_cdr(expr);  /* Skip past the "begin" */
    return_if_error(expr);

    /* There must always be at least one subexpression in a begin block. */
    do {
        subexpr = get_car(expr);
        return_if_error(subexpr);

        result = evaluate(env, subexpr);
        return_if_error(result);

        /* Take another step down the series of expressions to evaluate... */
        expr = get_cdr(expr);
    }
    while (!is_nil(expr));

    return result;
}


/*!
 * This function handles the let special form:  (let bindings body)
 *
 * This special form is rather complicated but is very useful for evaluating
 * temporary values once, binding them to names, and then reusing them.  The
 * full form is:
 *
 *     (let ((name1 expr1) (name2 expr2) ...) body_expr1 body_expr2 ...)
 *
 * Each name/value pair in the bindings section is evaluated in the current
 * context, and then a new child context is set up and all name/value pairs are
 * bound into the child context.  Then, each of the body expressions is
 * evaluated in the child context, and the result of the let expression is
 * simply the result of the last body-expression.
 *
 * This form could be desugared into a lambda:
 *
 *     ((lambda (name1 name2 ...) body_expr1 body_expr2 ...) expr1 expr2 ...)
 *
 * In other words, create a lambda with the binding-names as arguments and a
 * body containing the body-expressions, and then immediately apply it to the
 * binding-values.
 *
 * Although this stupid Scheme interpreter's implementation is probably not that
 * much faster than simply doing the transformation and evaluating the lambda,
 * Donnie decided to explicitly implement the let-evaluation manually, without a
 * transform.
 */
Value * eval_let(Environment *env, Value *expr) {
    Value *bindings, *body, *result;

    ListBuilder binding_names, binding_values;

    Environment *child_env;
    EvaluationContext *child_ctx;

    /* Need to register these so they don't get garbage-collected.  Everything
     * else is just references to stuff that's already registered somewhere.
     */
    evalctx_register(&binding_names.head);
    evalctx_register(&binding_values.head);
    
    /* Break apart the S-expression into its component parts. */

    bindings = get_cadr(expr);  /* First sublist contains bindings. */
    if (is_error(bindings)) {
        result = bindings;
        goto Done;
    }

    expr = get_cdr(expr);       /* Remainder of let expression is the body. */
    return_if_error(expr);
    body = get_cdr(expr);
    return_if_error(body);

    /*
     * Prepare the binding names and values.  These are all evaluated in the
     * context of the current environment.
     */

    init_list_builder(&binding_names);
    init_list_builder(&binding_values);

    while (!is_nil(bindings)) {
        Value *binding, *binding_name, *binding_expr, *binding_value;

        binding = get_car(bindings);
            
        /* Pull out binding name and store it in the name list. */
        binding_name = get_car(binding);
        if (!is_atom(binding_name))
            return make_error("binding names must be atoms");

        append_value_to_list(&binding_names, binding_name);

        /* Pull out the binding value, evaluate it, and store the result in the
         * value list.
         */
        binding_expr = get_cadr(binding);
        binding_value = evaluate(env, binding_expr);
        return_if_error(binding_value);

        append_value_to_list(&binding_values, binding_value);

        bindings = get_cdr(bindings);
    }
    
    /*
     * Create a new child environment, and set up all the bindings in the new
     * child environment.
     *
     * NOTE:  AFTER THIS POINT, DO NOT JUST RETURN if we hit an error!  We need
     *        to make sure we pop the child evaluation context we are about to
     *        create.
     */

    child_env = make_environment(env);
    child_ctx = push_new_evalctx(child_env, body);
    evalctx_register(&result);

    result = bind_names_values(child_env, binding_names.head, binding_values.head);
    if (is_error(result))
        goto Done;

    /*
     * Evaluate the body of the let expression in the context of the new child
     * environment.
     */

    /* There must always be at least one subexpression in a let block. */
    do {
        Value *body_expr = get_car(body);
        goto_done_if_error(body_expr);

        result = evaluate(child_env, body_expr);
        if (is_error(result))
            goto Done;

        /* Take another step down the series of expressions to evaluate... */
        body = get_cdr(body);
    }
    while (!is_nil(body));

Done:
    pop_evalctx(result);    /* Get rid of the child evaluation context. */

    return result;
}


/*!
 * This function handles the if special form:  (if condval trueval falseval)
 */
Value * eval_if(Environment *env, Value *expr) {
    Value *test_expr, *true_expr, *false_expr;
    Value *test_result, *final_result;

    /* Break apart the S-expression into its component parts. */

    expr = get_cdr(expr);  /* Step down the input expression. */
    return_if_error(expr);

    test_expr = get_car(expr);
    return_if_error(test_expr);

    expr = get_cdr(expr);  /* Step down the input expression. */
    return_if_error(expr);

    true_expr = get_car(expr);
    return_if_error(true_expr);

    expr = get_cdr(expr);  /* Step down the input expression. */
    return_if_error(expr);

    false_expr = get_car(expr);
    return_if_error(false_expr);

    /*
     * Evaluate 1st argument.  If it's true, evaluate and return 2nd argument.
     * Otherwise, evaluate and return 3rd argument.
     */

    test_result = evaluate(env, test_expr);
    return_if_error(test_result);

    if (is_true(test_result))
        final_result = evaluate(env, true_expr);
    else
        final_result = evaluate(env, false_expr);

    return final_result;
}


/*!
 * This function handles the and special form:  (and val val ...)
 *
 * This special form can take zero or more arguments.  If it receives zero
 * arguments then the result is automatically #t.  Otherwise, the arguments are
 * evaluated in sequence, and the first result that is false (i.e. #f) causes
 * evaluation to cease and the false result is returned.  If no results are
 * false then the last true value evaluated is returned.
 */
Value * eval_and(Environment *env, Value *expr) {
    Value *test_expr;
    Value *test_result = NULL;

    expr = get_cdr(expr);  /* Skip past the "and" */
    return_if_error(expr);
    
    while (!is_nil(expr)) {
        test_expr = get_car(expr);
        return_if_error(test_expr);

        test_result = evaluate(env, test_expr);
        return_if_error(test_result);

        if (is_false(test_result))
            break;

        /* Take another step down the expression to evaluate... */
        expr = get_cdr(expr);
    }

    /* If "and" got no arguments, result is defined to be true. */
    if (test_result == NULL)
        test_result = make_true();
    
    return test_result;
}


/*!
 * This function handles the or special form:  (or val val ...)
 *
 * This special form can take zero or more arguments.  If it receives zero
 * arguments then the result is automatically #f.  Otherwise, the arguments are
 * evaluated in sequence, and the first result that is true (i.e. anything other
 * than #f) causes evaluation to cease and the true result is returned.  If no
 * results are true then the last false value evaluated is returned.
 */
Value * eval_or(Environment *env, Value *expr) {
    Value *test_expr;
    Value *test_result = NULL;

    expr = get_cdr(expr);  /* Skip past the "or" */
    return_if_error(expr);
    
    while (!is_nil(expr)) {
        test_expr = get_car(expr);
        return_if_error(test_expr);
    
        test_result = evaluate(env, test_expr);
        return_if_error(test_result);

        if (is_true(test_result))
            break;

        /* Take another step down the expression to evaluate... */
        expr = get_cdr(expr);
    }

    /* If "or" got no arguments, result is defined to be false. */
    if (test_result == NULL)
        test_result = make_false();

    return test_result;
}


/*!
 * This function handles the cond special form:
 *     (cond (test) ...)
 *     (cond (test exp1 exp2 ...)
 *     (cond ... (else ...))
 */
Value * eval_cond(Environment *env, Value *expr) {
    Value *clause, *test_expr, *test_result;

    /* Skip past the "cond" atom. */
    expr = get_cdr(expr);
    return_if_error(expr);

    do {
        /* Extract the current clause of the cond. */
        clause = get_car(expr);
        return_if_error(clause);

        if (!is_cons_pair(clause))
            return make_error("cond clauses must be list expressions");

        test_expr = get_car(clause);  /* Get the actual expression. */
        return_if_error(test_expr);

        /* We use this clause if the condition is true, or if the clause is an
         * else clause.
         */

        if (is_atom(test_expr) && strcmp(test_expr->string_val, "else") == 0) {
            /* else clause must appear last in list of clauses. */
            if (!is_nil(get_cdr(expr)))
                return make_error("else clause must be last clause in cond");

            test_result = make_true();
        }
        else {
            /* Not an else-clause.  Evaluate the test.  If it's true, this is
             * our branch.
             */
            test_result = evaluate(env, test_expr);
            return_if_error(test_result);
        }

        if (is_true(test_result)) {

            clause = get_cdr(clause);
            return_if_error(clause);

            while (!is_nil(clause)) {

                test_expr = get_car(clause);  /* Get the actual expression. */
                return_if_error(test_expr);

                /* Evaluate the next expression in the clause. */
                test_result = evaluate(env, test_expr);
                return_if_error(test_result);

                clause = get_cdr(clause);
                return_if_error(clause);
            }

            /* All done! */
            return test_result;
        }

        /* Step forward to the next clause. */
        expr = get_cdr(expr);
        return_if_error(expr);
    }
    while (!is_nil(expr));

    return make_error("cond expression contains no matching branches!");
}




/*!
 * This function handles the define special form:
 *     (define x expr)
 *
 * The function also handles the sugared form of defining a lambda:
 *     (define (f x y z) body)
 *     (define (f x y z . w) body)
 *     (define (f . x) body)
 * These expressions are handled with the eval_sugared_define helper.
 */
Value * eval_define(Environment *env, Value *expr) {
    Value *name, *val;

    /* Break apart the S-expression into its component parts. */

    expr = get_cdr(expr);   /* Skip past the define atom. */
    return_if_error(expr);

    /* Name of binding that define will create. */
    name = get_car(expr);
    return_if_error(name);

    /* Special case:  is this a sugared lambda definition? */
    if (is_cons_pair(name))
        return eval_sugared_define(env, expr);

    /*
     * The rest of this function is focused on handling the normal "bind a
     * value to a name" version of define.
     */

    if (!is_atom(name))
        return make_error("first argument to define must be an atom");

    /* Get the expression that will evaluate to the value to store. */
    expr = get_cdr(expr);
    return_if_error(expr);

    val = get_car(expr);
    return_if_error(val);

    /* Evaluate the value expression to a result. */
    val = evaluate(env, val);
    return_if_error(val);
    if (!create_binding(env, name->string_val, val))
        return make_error("couldn't create specified binding!");

    return val;
}


/*!
 * This helper function is called from eval_define() to handle the sugared form
 * of defining a lambda:
 *     (define (f x y z) body)
 *     (define (f x y z . w) body)
 *     (define (f . x) body)
 */
Value * eval_sugared_define(Environment *env, Value *expr) {
    Value *func_spec, *body;
    Value *func_name, *func_args;
    Value *lambda;

    func_spec = get_car(expr);
    return_if_error(func_spec);

    func_name = get_car(func_spec);
    return_if_error(func_name);
    if (!is_atom(func_name))
        return make_error("function name in sugared define must be an atom");

    func_args = get_cdr(func_spec);
    return_if_error(func_args);
    if (!(is_atom(func_args) || is_cons_pair(func_args))) {
        return make_error(
            "function arguments in sugared define must be an atom or a list");
    }

    body = get_cdr(expr);
    return_if_error(body);

    lambda = make_lambda(env, func_args, body);
    return_if_error(lambda);
    if (!create_binding(env, func_name->string_val, lambda))
        return make_error("couldn't create specified binding!");

    return lambda;
}



/*!
 * This function handles the lambda special form:
 *     (lambda (x y z) body)
 *     (lambda (x y z . w) body)
 *     (lambda x body)
 */
Value * eval_lambda(Environment *env, Value *expr) {
    Value *arg_spec, *body;

    /* Break apart the S-expression into its component parts. */

    expr = get_cdr(expr);   /* Skip past the lambda atom. */
    return_if_error(expr);

    /* Argument specification for the lambda expression. */
    arg_spec = get_car(expr);
    return_if_error(arg_spec);

    body = get_cdr(expr);     /* Body is remainder of lambda expression. */
    return_if_error(body);

    //TODO:  if (!is_list(body))
    if (!is_cons_pair(body))
        return make_error("lambda body must be a list of scheme expressions");

    return make_lambda(env, arg_spec, body);
}


/*!
 * This function handles the quote special form:  (quote expr)
 *
 * The evaluation is very simple; the result is simply the unevaluated
 * expression.
 */
Value * eval_quote(Environment *env, Value *expr) {
    Value *result;

    expr = get_cdr(expr);   /* Skip past the quote atom. */
    return_if_error(expr);

    result = get_car(expr);
    return_if_error(result);

    /* If we get here then expr has to be a cons-pair. */
    assert(is_cons_pair(expr));
    if (!is_nil(get_cdr(expr)))
        return make_error("quote requires only a single argument");

    return result;
}


/*!
 * This function handles the set! special form:  (set! name expr)
 *
 * The expression is evaluated normally, and then the closest existing binding
 * is updated with the new value.  Note that set! NEVER creates a new binding!
 */
Value * eval_set_bang(Environment *env, Value *expr) {
    Value *name, *val;

    expr = get_cdr(expr);   /* Skip past the set! atom. */
    return_if_error(expr);

    name = get_car(expr);
    return_if_error(name);

    if (!is_atom(name))
        return make_error("first argument to set! must be a name");

    expr = get_cdr(expr);   /* Now for the actual value to set the name to. */

    val = get_car(expr);

    if (!is_nil(get_cdr(expr)))
        return make_error("set! requires exactly two arguments");

    /* Evaluate the value, then update the binding! */

    val = evaluate(env, val);
    return_if_error(val);

    if (!update_binding(env, name->string_val, val))
        return make_error("no existing binding to update!");

    return val;
}



