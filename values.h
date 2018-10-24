#ifndef VALUES_H
#define VALUES_H


#include "types.h"
#include <stdio.h>


void raw_print_value(const Value *v);

void print_value(FILE *f, const Value *v);


Value * make_error(const char *str, ...) __attribute__((format (printf, 1, 2)));

Value * make_atom(const char *str);

Value * make_bool(int b);
Value * make_true(void);
Value * make_false(void);

Value * make_string(const char *str);
Value * make_float(float f);

Value * make_nil(void);
Value * make_cons(Value *car, Value *cdr);

Value * make_lambda(struct Environment *parent_env, Value *arg_spec, Value *body);
Value * make_native_lambda(struct Environment *parent_env, NativeLambda func);

int is_atom(Value *v);

int is_bool(Value *v);
int is_true(Value *v);
int is_false(Value *v);

int is_error(Value *v);

int is_float(Value *v);

int is_string(Value *v);

int is_cons_pair(Value *v);

int is_nil(Value *v);

int is_lambda(Value *v);


Value * get_car(Value *cons);
Value * get_cdr(Value *cons);
Value * get_cadr(Value *cons);


void set_car(Value *cons, Value *v);
void set_cdr(Value *cons, Value *v);


int list_length(Value *cons);


#define return_if_error(v) { if (is_error(v)) return (v); }
#define goto_done_if_error(v) { if (is_error(v)) { result = (v); goto Done; } }


/*!
 * This is a helper-struct used for building up lists in C code, since it's
 * pretty onerous to do so otherwise...
 */
typedef struct ListBuilder {
    /*!
     * A pointer to the head of the list.  This will either be nil (NOT NULL!)
     * or a cons pair.
     */
    Value *head;
    
    /*!
     * A pointer to the last cons pair in the list.  This will either be NULL or
     * the last cons pair in the list.
     */
    Value *tail;

} ListBuilder;

void init_list_builder(ListBuilder *builder);
void append_value_to_list(ListBuilder *builder, Value *v);



#endif /* VALUES_H */


