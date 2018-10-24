/*! \file
 * This file contains all important type-definitions for the CS24 Scheme
 * interpreter.  Thus, it is referenced by many other files in the codebase.
 */


#ifndef TYPES_H
#define TYPES_H

#include "ptr_vector.h"


/*! A struct for tracking variable-bindings within an environment. */
typedef struct Binding {
    char *name;             /*!< The name of the binding. */
    struct Value *value;    /*!< The value that is bound to the name. */
} Binding;


/*!
 * An environment simply stores a collection of name/value bindings, as well as
 * a pointer to the parent environment.  When a (non-native) procedure is
 * applied to its arguments, a new environment is created, and the arguments and
 * their corresponding values are stored into the environment's bindings.  A
 * procedure can use statements like (define ...), (set! ...), etc. to
 * manipulate the bindings in the current environment.
 */
typedef struct Environment {

    /* Variable-bindings are stored in a growable vector. */

    /*!
     * The number of bindings we have allocated space for, although this is
     * usually more than the actual number of bindings we currently have.
     */
    int capacity;

    /*! The number of bindings we actually currently have. */
    int num_bindings;

    /*! An array of "binding" structs, which store the name/value pairs. */
    Binding *bindings;


    /*!
     * Pointer to parent environment.  This will be set to NULL for the global
     * environment.
     */
    struct Environment *parent_env;

    /*! For garbage collection. */
    int marked;

} Environment;


/*!
 * This enumeration specifies all the different kinds of Scheme values allowed
 * in the interpreter.  It is used as the tag for the tagged-value data type.
 */
typedef enum Type {
    T_Error,
    T_Nil,
    T_Atom,
    T_Boolean,
    T_String,
    T_Float,
    T_Lambda,
    T_ConsPair
} Type;


/*!
 * A cons pair is a simple composite data type in Scheme, consisting of two
 * pointers to Value structs.  The first value is called the "car", and the
 * second value is called the "cdr."
 */
typedef struct ConsPair {
    struct Value *p_car;    /*!< The first value in the cons pair. */
    struct Value *p_cdr;    /*!< The second value in the cons pair. */
} ConsPair;


/*!
 * This is a tagged data type used to represent all the different kinds of
 * values that this Scheme interpreter supports.  The type field indicates the
 * kind of value, and the union member can be used to represent all the
 * different kinds of values without using up a large amount of memory.
 */
typedef struct Value {
    /*! The type of this value. */
    Type type;

    /*!
     * A union of all the different possible values.  The specific value to use
     * is dependent on the type tag.  A T_Nil type doesn't have a value
     * represented in this union.
     */
    union {
        char  *string_val;           /* T_Error, T_Atom, T_String */
        int    bool_val;             /* T_Boolean */
        float  float_val;            /* T_Float */
        struct Lambda *lambda_val;   /* T_Lambda */
        ConsPair cons_val;           /* T_ConsPair */
    };

    /*! For garbage collection. */
    int marked;

} Value;


/*!
 * This type-definition declares the interface used for calling procedures that
 * are implemented in C, hence the name "native lambda."
 */
typedef Value * (*NativeLambda)(int num_args, Value *inputs);


/*!
 * This type represents a procedure in the Scheme interpreter.  Procedures can
 * be implemented either as Scheme code, or as native C functions that follow
 * the NativeLambda function signature.
 */
typedef struct Lambda {

    /*! List of argument-name symbols, as a Scheme list structure. */
    Value *arg_spec;


    /*!
     * This flag indicates whether the lambda has a native implementation (i.e.
     * is a built-in function), or whether it is interpreted.
     */
    int native_impl;


    union {
        /*!
         * If the lambda is a built-in function, this is a function-pointer to
         * the lambda.
         */
        NativeLambda func;

        /*! If the lambda is interpreted, this is the body of the lambda. */
        Value *body;
    };


    /*! The parent environment of the lambda. */
    struct Environment *parent_env;

    /*! For garbage collection. */
    int marked;

} Lambda;


/*!
 * This struct is used to represent the details of a single expression
 * evaluation, including the environment being used, the expression being
 * evaluated, and the local variables used during the evaluation.  Since
 * evaluations frequently require nested evaluations, these contexts are
 * managed in a stack available from the evaluator.
 */
typedef struct EvaluationContext {

    /*! The environment being used for the current evaluation. */
    Environment *current_env;

    /*! The expression being evaluated in this context. */
    Value *expression;

    /*!
     * An evaluation frequently requires multiple nested evaluations, such as
     * when a procedure is being called and its operands need to be evaluated.
     * In these cases, the result of the nested evaluation must be stored
     * somewhere before the garbage collector can be invoked, and that is best
     * done in the enclosing evaluation's context.  Thus, the result of a call
     * to the evaluate() function is stored in this member of the parent context
     * before the function performs garbage collection and then returns.
     */
    Value *child_eval_result;

    /*!
     * Pointer-vector of intermediate results.  Elements are pointers to
     * Value-struct pointers.
     */
    PtrVector local_vals;

} EvaluationContext;


#endif /* TYPES_H */

