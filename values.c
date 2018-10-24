#include <assert.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>

#include "values.h"
#include "alloc.h"
#include "evaluator.h"


static char *value_type_names[] = {
    "T_Error", "T_Nil", "T_Atom", "T_Boolean", "T_String", "T_Float",
    "T_Lambda", "T_ConsPair"
};



void raw_print_value(const Value *v) {
    if (v == NULL) {
        printf("[NULL Value]\n");
        return;
    }

    switch (v->type) {

    case T_Nil:
        printf("nil");
        break;

    case T_Boolean:
        printf("Value[%s:%d]\n", value_type_names[v->type], v->bool_val);
        break;

    case T_Atom:
    case T_String:
        printf("Value[%s:%s]\n", value_type_names[v->type], v->string_val);
        break;

    case T_Float:
        printf("Value[%s:%f]\n", value_type_names[v->type], v->float_val);
        break;

    case T_ConsPair:
        printf("Value[%s:0x%08X,0x%08X]\n", value_type_names[v->type],
            (unsigned int) v->cons_val.p_car, (unsigned int) v->cons_val.p_cdr);
        break;

    default:
        printf("[UNKNOWN Value]\n");
    }
}


void print_value(FILE *f, const Value *v) {
    assert(f != NULL);

    if (v == NULL) {
        fprintf(f, "NULL");
        return;
    }

    switch (v->type) {

    case T_Nil:
        fprintf(f, "nil");
        break;

    case T_Boolean:
        fprintf(f, "%s", (v->bool_val ? "#t" : "#f"));
        break;

    case T_Atom:
    case T_String:
        fprintf(f, "%s", v->string_val);
        break;

    case T_Float:
        fprintf(f, "%g", v->float_val);
        break;

    case T_Lambda:
        if (!v->lambda_val->native_impl) {
            fprintf(f, "#lambda[args=");
            print_value(f, v->lambda_val->arg_spec);
            fprintf(f, " body=");
            print_value(f, v->lambda_val->body);
            fprintf(f, "]");
        }
        else {
            fprintf(f, "#native_lambda[0x%08x]",
                    (unsigned int) v->lambda_val->func);
        }
        break;

    case T_ConsPair:
        {
            Value *car;
            Value *cdr;
            int first = 1;

            fprintf(f, "(");

            while (1) {
                car = v->cons_val.p_car;
                cdr = v->cons_val.p_cdr;

                if (!first)
                    fprintf(f, " ");
                else
                    first = 0;

                print_value(f, car);

                if (cdr != NULL && cdr->type == T_ConsPair)
                    v = cdr;
                else
                    break;
            }

            assert(cdr == NULL || cdr->type != T_ConsPair);

            if (cdr != NULL && cdr->type == T_Nil) {
                fprintf(f, ")");
            }
            else {
                fprintf(f, " . ");
                print_value(f, cdr);
                fprintf(f, ")");
            }
        }
        break;

    case T_Error:
        fprintf(f, "ERROR:  %s", v->string_val);
        break;

    default:
        fprintf(f, "UNKNOWN");
    }

}




/*!
 * Given an error string, this function creates a new Value object of type
 * T_Error.  Note that the passed-in string is NOT owned by the
 * new Value object; rather, the input is copied.
 */
Value * make_error(const char *str, ...) {

    va_list ap;
    char buf[200];
    Value *v = alloc_value();

    va_start(ap, str);
    vsnprintf(buf, sizeof(buf), str, ap);
    va_end(ap);

    v->type = T_Error;
    v->string_val = strdup(buf);

    return v;
}



/*! Returns a pointer to the nil value. */
Value * make_nil() {
    Value *v = alloc_value();

    v->type = T_Nil;

    return v;
}


/*!
 * Given a string representation of an atom, this function creates a new Value
 * object of type T_Atom.  Note that the passed-in string is NOT owned by the
 * new Value object; rather, the input is copied.
 */
Value * make_atom(const char *str) {
    Value *v = alloc_value();

    v->type = T_Atom;
    v->string_val = strdup(str);

    return v;
}


/*!
 * Given a Boolean value, this function creates a new Value object of type
 * T_Boolean.
 */
Value * make_bool(int b) {
    Value *v = alloc_value();

    v->type = T_Boolean;
    v->bool_val = b;

    return v;
}

/*! This function creates a new Boolean true value. */
Value * make_true() {
    return make_bool(1);
}

/*! This function creates a new Boolean false value. */
Value * make_false() {
    return make_bool(0);
}


/*!
 * Given a string-literal value, this function creates a new Value object of
 * type T_String.  Note that the passed-in string is NOT owned by the
 * new Value object; rather, the input is copied.
 */
Value * make_string(const char *str) {
    Value *v = alloc_value();

    v->type = T_String;
    v->string_val = strdup(str);

    return v;
}


Value * make_float(float f) {
    Value *v = alloc_value();

    v->type = T_Float;
    v->float_val = f;

    return v;
}


Value * make_cons(Value *car, Value *cdr) {
    Value *v = alloc_value();

    v->type = T_ConsPair;
    v->cons_val.p_car = car;
    v->cons_val.p_cdr = cdr;

    return v;
}


Value * make_lambda(Environment *parent_env, Value *arg_spec, Value *body) {
    Value *v;
    Lambda *f;

    /* Every lambda expression MUST have a parent environment. */
    assert(parent_env != NULL);
    assert(arg_spec != NULL);
    assert(body != NULL);

    /* The argument-spec must either be an atom, a list of atoms, or an
     * improper list of atoms.  Otherwise the spec is invalid.
     */
    if (!is_atom(arg_spec)) {
        Value *arg_iter = arg_spec;

        do {
            if (!is_cons_pair(arg_iter) || !is_atom(get_car(arg_iter))) {
                return make_error("lambda argument must be an atom, "
                                  "or a list of atoms");
            }

            arg_iter = get_cdr(arg_iter);
        }
        while (!(is_nil(arg_iter) || is_atom(arg_iter)));
    }

    v = alloc_value();
    f = alloc_lambda();

    f->parent_env = parent_env;
    f->arg_spec = arg_spec;
    f->native_impl = 0;       /* Interpreted lambda. */
    f->body = body;

    v->type = T_Lambda;
    v->lambda_val = f;

    return v;
}


Value * make_native_lambda(Environment *parent_env, NativeLambda func) {
    Value *v;
    Lambda *f;

    /* Every lambda expression MUST have a parent environment. */
    assert(parent_env != NULL);
    assert(func != NULL);

    v = alloc_value();
    f = alloc_lambda();

    f->parent_env = parent_env;
    /* f->arg_spec = arg_spec; */
    f->native_impl = 1;       /* Native lambda. */
    f->func = func;

    v->type = T_Lambda;
    v->lambda_val = f;

    return v;
}




int is_atom(Value *v) {
    return (v != NULL && v->type == T_Atom);
}

int is_bool(Value *v) {
    return (v != NULL && v->type == T_Boolean);
}

/*!
 * Returns nonzero if the argument is any value other than the Scheme value #f.
 * Only the Scheme value #f counts as false.  Every other value, including nil,
 * counts as true.
 */
int is_true(Value *v) {
    return !is_false(v);
}

/*!
 * Returns nonzero if the argument is the Scheme value #f.  Only the Scheme
 * value #f counts as false.  Every other value, including nil, counts as true.
 */
int is_false(Value *v) {
    return (v != NULL && v->type == T_Boolean && v->bool_val == 0);
}

int is_error(Value *v) {
    return (v != NULL && v->type == T_Error);
}

int is_float(Value *v) {
    return (v != NULL && v->type == T_Float);
}

int is_string(Value *v) {
    return (v != NULL && v->type == T_String);
}

int is_cons_pair(Value *v) {
    return (v != NULL && v->type == T_ConsPair);
}

int is_nil(Value *v) {
    return (v != NULL && v->type == T_Nil);
}


int is_lambda(Value *v) {
    return (v != NULL && v->type == T_Lambda);
}




Value * get_car(Value *cons) {
    if (!is_cons_pair(cons))
        return make_error("argument to get_car must be a cons pair");

    return cons->cons_val.p_car;
}



Value * get_cdr(Value *cons) {
    if (!is_cons_pair(cons))
        return make_error("argument to get_cdr must be a cons pair");

    return cons->cons_val.p_cdr;
}


Value * get_cadr(Value *cons) {
    Value *cdr;

    if (!is_cons_pair(cons))
        return make_error("argument to get_cadr must be a cons pair");

    cdr = get_cdr(cons);
    if (!is_cons_pair(cdr))
        return make_error("cdr of argument to get_cadr must be a cons pair");

    return get_car(cdr);
}




void set_car(Value *cons, Value *v) {
    assert(cons != NULL);
    assert(cons->type == T_ConsPair);

    assert(v != NULL);

    cons->cons_val.p_car = v;
}

void set_cdr(Value *cons, Value *v) {
    assert(cons != NULL);
    assert(cons->type == T_ConsPair);

    assert(v != NULL);

    cons->cons_val.p_cdr = v;
}


int list_length(Value *cons) {
    int length = 0;

    if (!is_cons_pair(cons) && !is_nil(cons))
        return -1;

    while (!is_nil(cons)) {
        cons = get_cdr(cons);

        if (!is_cons_pair(cons) && !is_nil(cons))
            return -1;

        length++;
    }

    return length;
}


void init_list_builder(ListBuilder *builder) {
    assert(builder != NULL);
    
    builder->head = make_nil();
    builder->tail = builder->head;
}

void append_value_to_list(ListBuilder *builder, Value *v) {
    assert(builder != NULL);
    assert(v != NULL);

    if (is_nil(builder->head)) {
        assert(builder->tail == builder->head);
        
        builder->head = make_cons(v, builder->head);
        builder->tail = builder->head;
    }
    else {
        Value *new_cons, *nil_val;
        
        nil_val = get_cdr(builder->tail);
        assert(is_nil(nil_val));

        new_cons = make_cons(v, nil_val);
        set_cdr(builder->tail, new_cons);
    }
}

