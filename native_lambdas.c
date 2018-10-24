#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

#include "native_lambdas.h"
#include "values.h"
#include "repl.h"                /* for exec_file */


/*!
 * Performs equality check between two Value objects containing floating-point
 * numbers.  This function is used as an argument to do_comparison().
 */
int fn_equal(Value *v1, Value *v2) {
    assert(v1 != NULL);
    assert(is_float(v1));
    assert(v2 != NULL);
    assert(is_float(v2));

    return (v1->float_val == v2->float_val);
}

/*!
 * Performs less-than check between two Value objects containing floating-point
 * numbers.  This function is used as an argument to do_comparison().
 */
int fn_less_than(Value *v1, Value *v2) {
    assert(v1 != NULL);
    assert(is_float(v1));
    assert(v2 != NULL);
    assert(is_float(v2));

    return (v1->float_val < v2->float_val);
}

/*!
 * Performs greater-than check between two Value objects containing
 * floating-point numbers.  This function is used as an argument to
 * do_comparison().
 */
int fn_greater_than(Value *v1, Value *v2) {
    assert(v1 != NULL);
    assert(is_float(v1));
    assert(v2 != NULL);
    assert(is_float(v2));

    return (v1->float_val > v2->float_val);
}

/*!
 * Performs less-or-equal (i.e. at-most) check between two Value objects
 * containing floating-point numbers.  This function is used as an argument to
 * do_comparison().
 */
int fn_less_equal(Value *v1, Value *v2) {
    assert(v1 != NULL);
    assert(is_float(v1));
    assert(v2 != NULL);
    assert(is_float(v2));

    return (v1->float_val <= v2->float_val);
}

/*!
 * Performs greater-or-equal (i.e. at-least) check between two Value objects
 * containing floating-point numbers.  This function is used as an argument to
 * do_comparison().
 */
int fn_greater_equal(Value *v1, Value *v2) {
    assert(v1 != NULL);
    assert(is_float(v1));
    assert(v2 != NULL);
    assert(is_float(v2));

    return (v1->float_val >= v2->float_val);
}


/*!
 * This helper function is used to implement the Scheme built-in numeric
 * comparison functions.  It can handle two or more floating-point arguments,
 * and applies the specified comparison function to successive pairs of
 * arguments as long as the comparison result is true.
 */
Value * do_comparison(int num_args, Value *args,
                      int (*compare)(Value *, Value *)) {
    Value *v1, *v2;

    if (num_args < 2)
        return make_error("comparison requires at least two arguments");

    v2 = get_car(args);
    if (!is_float(v2))
        return make_error("comparison requires numeric values");

    do {
        v1 = v2;
        args = get_cdr(args);
        v2 = get_car(args);

        if (!is_float(v2))
            return make_error("comparison requires numeric values");

        /* If these values aren't in order, we can stop early! */
        if (!compare(v1, v2))
            return make_false();
    }
    while (!is_nil(get_cdr(args)));

    /* Make sure arguments-list was constructed properly by the evaluator. */
    assert(is_nil(get_cdr(args)));

    /* If we made it through all the values then they are in the proper order,
     * and we can return #t.
     */
    return make_true();
}


/*!
 * This function implements the Scheme built-in function "=" for numeric
 * equality comparisons.
 */
Value * scheme_numeric_equals(int num_args, Value *args) {
    return do_comparison(num_args, args, fn_equal);
}

/*!
 * This function implements the Scheme built-in function "&lt;" for numeric
 * less-than comparisons.
 */
Value * scheme_numeric_less_than(int num_args, Value *args) {
    return do_comparison(num_args, args, fn_less_than);
}

/*!
 * This function implements the Scheme built-in function "&gt;" for numeric
 * greater-than comparisons.
 */
Value * scheme_numeric_greater_than(int num_args, Value *args) {
    return do_comparison(num_args, args, fn_greater_than);
}

/*!
 * This function implements the Scheme built-in function "&lt;=" for numeric
 * less-or-equal (i.e. "at most") comparisons.
 */
Value * scheme_numeric_less_equal(int num_args, Value *args) {
    return do_comparison(num_args, args, fn_less_equal);
}

/*!
 * This function implements the Scheme built-in function "&gt;=" for numeric
 * greater-or-equal (i.e. "at least") comparisons.
 */
Value * scheme_numeric_greater_equal(int num_args, Value *args) {
    return do_comparison(num_args, args, fn_greater_equal);
}


Value * type_predicate_helper(const char *name, int num_args, Value *args,
        int (*predicate)(Value *)) {

    if (num_args != 1)
        return make_error("%s takes exactly one argument", name);

    assert(is_cons_pair(args));
    return make_bool(predicate(get_car(args)));
}


/*!
 *
 */
Value * scheme_is_boolean(int num_args, Value *args) {
    return type_predicate_helper("boolean?", num_args, args, is_bool);
}


/*!
 *
 */
Value * scheme_is_number(int num_args, Value *args) {
    return type_predicate_helper("number?", num_args, args, is_float);
}


/*!
 *
 */
Value * scheme_is_pair(int num_args, Value *args) {
    return type_predicate_helper("pair?", num_args, args, is_cons_pair);
}


/*!
 *
 */
Value * scheme_is_procedure(int num_args, Value *args) {
    return type_predicate_helper("procedure?", num_args, args, is_lambda);
}


/*!
 *
 */
Value * scheme_is_string(int num_args, Value *args) {
    return type_predicate_helper("string?", num_args, args, is_string);
}


/*!
 *
 */
Value * scheme_is_symbol(int num_args, Value *args) {
    return type_predicate_helper("symbol?", num_args, args, is_atom);
}



/*!
 * This function implements the Scheme built-in function "+" for numeric
 * addition.
 */
Value * scheme_add(int num_args, Value *args) {
    Value *v;
    float result = 0;

    while (is_cons_pair(args)) {
        v = get_car(args);
        if (!is_float(v))
            return make_error("invalid argument to +");

        result += v->float_val;

        args = get_cdr(args);
    }

    if (!is_nil(args))
        return make_error("invalid argument to +");

    return make_float(result);
}


/*!
 * This function implements the Scheme built-in function "-" for numeric
 * subtraction.
 */
Value * scheme_sub(int num_args, Value *args) {
    Value *v;
    float result;

    if (num_args == 0)
        return make_error("- requires at least one argument");

    assert(is_cons_pair(args));
    v = get_car(args);
    if (!is_float(v))
        return make_error("invalid argument to -");

    result = v->float_val;

    args = get_cdr(args);
    if (is_nil(args)) {
        /* - functions as unary negate, when given one argument. */
        result = -result;
    }
    else {
        while (is_cons_pair(args)) {
            v = get_car(args);
            if (!is_float(v))
                return make_error("invalid argument to -");

            result -= v->float_val;

            args = get_cdr(args);
        }

        if (!is_nil(args))
            return make_error("invalid argument to -");
    }

    return make_float(result);
}


/*!
 * This function implements the Scheme built-in function "*" for numeric
 * multiplication.
 */
Value * scheme_mul(int num_args, Value *args) {
    Value *v;
    float result = 1;

    while (is_cons_pair(args)) {
        v = get_car(args);
        if (!is_float(v))
            return make_error("invalid argument to *");

        result *= v->float_val;

        args = get_cdr(args);
    }

    if (!is_nil(args))
        return make_error("invalid argument to *");

    return make_float(result);
}


/*!
 * This function implements the Scheme built-in function "/" for numeric
 * division.
 */
Value * scheme_div(int num_args, Value *args) {
    Value *v;
    float result;

    if (num_args == 0)
        return make_error("/ requires at least one argument");

    assert(is_cons_pair(args));
    v = get_car(args);
    if (!is_float(v))
        return make_error("invalid argument to /");

    result = v->float_val;

    args = get_cdr(args);
    if (is_nil(args)) {
        /* / functions as multiplicative inverse, when given one argument. */
        
        if (result == 0)
            return make_error("divide by zero");

        result = 1.0 / result;
    }
    else {
        while (is_cons_pair(args)) {
            v = get_car(args);
            if (!is_float(v))
                return make_error("invalid argument to /");

            if (v->float_val == 0)
                return make_error("divide by zero");
        
            result /= v->float_val;

            args = get_cdr(args);
        }

        if (!is_nil(args))
            return make_error("invalid argument to /");
    }

    return make_float(result);
}


/*!
 * This function implements the Scheme built-in function "cons", which creates a
 * new cons-pair with the specified contents.
 */
Value * scheme_cons(int num_args, Value *args) {
    if (num_args != 2)
        return make_error("cons takes exactly two arguments");

    return make_cons(get_car(args), get_cadr(args));
}


/*!
 * This function implements the Scheme built-in function "car", which returns
 * the first value in a cons-pair.
 */
Value * scheme_car(int num_args, Value *args) {
    Value *v;

    if (num_args != 1)
        return make_error("car takes exactly one argument");

    v = get_car(args);   /* Extract the first (and only) argument to car. */
    if (!is_cons_pair(v))
        return make_error("argument to car must be a cons pair");

    return get_car(v);
}


/*!
 * This function implements the Scheme built-in function "cdr", which returns
 * the second value in a cons-pair.
 */
Value * scheme_cdr(int num_args, Value *args) {
    Value *v;

    if (num_args != 1)
        return make_error("cdr takes exactly one argument");

    v = get_car(args);   /* Extract the first (and only) argument to cdr. */
    if (!is_cons_pair(v))
        return make_error("argument to cdr must be a cons pair");

    return get_cdr(v);
}


/*!
 * This function implements the Scheme built-in function "list", which returns
 * a list containing its arguments.
 */
Value * scheme_list(int num_args, Value *args) {
    /* Easy implementation!!! */
    return args;
}


/*!
 * This function implements the Scheme built-in function "length", which returns
 * the length of a proper list.
 */
Value * scheme_length(int num_args, Value *args) {
    int n;
    
    if (num_args != 1)
        return make_error("length requires exactly one argument");

    /* Function returns -1 if argument isn't a proper list. */
    n = list_length(get_car(args));
    
    if (n == -1)
        return make_error("argument to length must be a proper list");
    
    return make_float(n);
}


/*!
 * This function implements the Scheme built-in function "eq?", which performs
 * an object-identity check between Scheme values.
 */
Value * scheme_eq(int num_args, Value *args) {
    Value *v1, *v2;
    int result;

    if (num_args != 2)
        return make_error("eq? requires exactly two arguments");

    v1 = get_car(args);
    v2 = get_car(get_cdr(args));

    if (v1->type != v2->type)
        return make_false();

    switch (v1->type) {
    case T_Nil:
        result = 1;
        break;

    case T_Atom:
    case T_String:
        result = (strcmp(v1->string_val, v2->string_val) == 0);
        break;

    case T_Boolean:
        assert(v1->bool_val == 1 || v1->bool_val == 0);
        assert(v2->bool_val == 1 || v2->bool_val == 0);
        result = (v1->bool_val == v2->bool_val);
        break;

    case T_Float:
        result = (v1->float_val == v2->float_val);
        break;

    case T_ConsPair:
    case T_Lambda:
        result = (v1 == v2);
        break;
    default:
        result = 0;
    }

    return make_bool(result);
}


/*!
 * This is a helper function for the equal? function implementation, since we
 * need to recurse when analyzing structures of cons pairs, and also with
 * lambdas.
 */
int fn_value_equality(Value *v1, Value *v2) {
    int result;

    if (v1->type != v2->type)
        return 0;

    switch (v1->type) {
    case T_Nil:
        result = 1;
        break;

    case T_Atom:
    case T_String:
        result = (strcmp(v1->string_val, v2->string_val) == 0);
        break;

    case T_Boolean:
        assert(v1->bool_val == 1 || v1->bool_val == 0);
        assert(v2->bool_val == 1 || v2->bool_val == 0);
        result = (v1->bool_val == v2->bool_val);
        break;

    case T_Float:
        result = (v1->float_val == v2->float_val);
        break;

    case T_ConsPair:
        if (v1 == v2) {  /* Just in case we are lucky, do this fast. */
            result = 1;
        }
        else {
            result = fn_value_equality(get_car(v1), get_car(v2)) &&
                     fn_value_equality(get_cdr(v1), get_cdr(v2));
        }

        break;

    case T_Lambda:
        result = 0;
        if (v1 == v2) {  /* Just in case we are lucky, do this fast. */
            result = 1;
        }
        else if (v1->lambda_val->native_impl && v2->lambda_val->native_impl) {
            /* Same function-pointers means same lambda expression.
             * (Actually, if this is the case, previous test will catch it,
             * since each lambda has exactly one Value object referring to it.)
             */
            result = (v1->lambda_val->func == v2->lambda_val->func);
        }
        else if (!v1->lambda_val->native_impl && !v2->lambda_val->native_impl) {
            /* Gotta compare the arguments and function bodies. */

            result = (v1->lambda_val->parent_env == v2->lambda_val->parent_env);
            result = result && fn_value_equality(v1->lambda_val->arg_spec,
                                                 v2->lambda_val->arg_spec);
            result = result && fn_value_equality(v1->lambda_val->body,
                                                 v2->lambda_val->body);
        }

        break;

    default:
        result = 0;
    }

    return result;
}


/*!
 * This function implements the Scheme built-in function "equal?", which
 * performs a value-equality check between Scheme values.
 */
Value * scheme_equal(int num_args, Value *args) {
    Value *v1, *v2;

    if (num_args != 2)
        return make_error("eq? requires exactly two arguments");

    v1 = get_car(args);
    v2 = get_car(get_cdr(args));

    return make_bool(fn_value_equality(v1, v2));
}


/*!
 * This function implements the Scheme built-in function "set-car!", which
 * performs in-place mutation of the first value in a cons-pair.
 */
Value * scheme_set_car(int num_args, Value *args) {
    Value *target, *val;

    if (num_args != 2)
        return make_error("set-car! requires exactly two arguments");

    target = get_car(args);
    return_if_error(target);

    if (!is_cons_pair(target))
        return make_error("first argument to set-car! must be a cons pair");

    val = get_car(get_cdr(args));
    return_if_error(val);

    set_car(target, val);

    return val;
}


/*!
 * This function implements the Scheme built-in function "set-cdr!", which
 * performs in-place mutation of the second value in a cons-pair.
 */
Value * scheme_set_cdr(int num_args, Value *args) {
    Value *target, *val;

    if (num_args != 2)
        return make_error("set-cdr! requires exactly two arguments");

    target = get_car(args);
    return_if_error(target);

    if (!is_cons_pair(target))
        return make_error("first argument to set-cdr! must be a cons pair");

    val = get_car(get_cdr(args));
    return_if_error(val);

    set_cdr(target, val);

    return val;
}


Value * scheme_display(int num_args, Value *args) {

    if (num_args == 0) {
        printf("\n");
    }
    else {
        while (!is_nil(args)) {
            Value *v = get_car(args);
            print_value(stdout, v);
            args = get_cdr(args);
        }
        printf("\n");
    }
    
    return NULL;
}


/*!
 * This function generates an error result from a single string argument
 * specifying the error message.
 */
Value * scheme_error(int num_args, Value *args) {
    Value *msg;

    if (num_args != 1)
        return make_error("error currently only supports one argument");

    msg = get_car(args);
    return_if_error(msg);

    if (!is_string(msg))
        return make_error("argument to error must be a string");

    return make_error("%s", msg->string_val);
}


/*!
 * This function seeds the random number generator, either with a single numeric
 * argument, or with the current time value if no argument is provided.  The
 * argument will be a float, but we cast it to an unsigned integer in here.
 */
Value * scheme_srandom(int num_args, Value *args) {
    Value *seed, *result;
    unsigned int seed_val;

    if (num_args == 0) {
        seed_val = time(NULL);
    }
    else if (num_args == 1) {
        seed = get_car(args);
        return_if_error(seed);

        if (!is_float(seed))
            return make_error("invalid argument to srandom");
    
        seed_val = (unsigned) seed->float_val;
    }
    else {
        return make_error("srandom takes zero or one arguments");
    }

    result = make_float((float) seed_val);
    return_if_error(result);

    srandom(seed_val);

    return result;
}
 
 
/*!
 * This function returns a random number from the random generator.
 */
Value * scheme_random(int num_args, Value *args) {
    Value *max = NULL;
    long rand_val;

    if (num_args == 1) {
        max = get_car(args);
        if (!is_float(max))
            return make_error("argument to random must be a number");
    }
    else if (num_args > 1) {
        return make_error("random takes zero or one arguments");
    }

    rand_val = random();
    if (max != NULL)
        rand_val %= (int) max->float_val;
        
    return make_float((float) rand_val);
}


/*!
 * This function returns the current time in seconds from the epoch, as the C
 * time() function returns.
 */
Value * scheme_time(int num_args, Value *args) {
    Value *result;

    if (num_args != 0)
        return make_error("time takes zero arguments");

    result = make_float((float) time(NULL));
    return result;
}


/*!
 * This function computes the square-root of the input argument.
 */
Value * scheme_sqrt(int num_args, Value *args) {
    Value *input, *result;

    if (num_args == 1) {
        input = get_car(args);
        return_if_error(input);

        if (!is_float(input))
            return make_error("invalid argument to sqrt");
    }
    else {
        return make_error("sqrt takes one argument");
    }

    result = make_float(sqrtf(input->float_val));
    return_if_error(result);

    return result;
}


/*!
 * This function evaluates a Scheme file in the context of the global
 * environment.
 */
Value * scheme_eval_file(int num_args, Value *args) {
    Value *filename;

    if (num_args != 1)
        return make_error("eval-file takes exactly one string argument");

    filename = get_car(args);
    if (!is_string(filename))
        return make_error("eval-file takes exactly one string argument");

    if (!exec_file(filename->string_val)) {
        return make_error(
            "eval-file failed for some reason (probably your fault)");
    }
    
    return make_true();
}


