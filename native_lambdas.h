#ifndef NATIVE_LAMBDAS_H
#define NATIVE_LAMBDAS_H


#include "types.h"


Value * scheme_eq(int num_args, Value *args);
Value * scheme_equal(int num_args, Value *args);

Value * scheme_numeric_equals(int num_args, Value *args);
Value * scheme_numeric_less_than(int num_args, Value *args);
Value * scheme_numeric_greater_than(int num_args, Value *args);
Value * scheme_numeric_less_equal(int num_args, Value *args);
Value * scheme_numeric_greater_equal(int num_args, Value *args);

Value * scheme_is_boolean(int num_args, Value *args);
Value * scheme_is_number(int num_args, Value *args);
Value * scheme_is_pair(int num_args, Value *args);
Value * scheme_is_procedure(int num_args, Value *args);
Value * scheme_is_string(int num_args, Value *args);
Value * scheme_is_symbol(int num_args, Value *args);

Value * scheme_add(int num_args, Value *args);
Value * scheme_sub(int num_args, Value *args);
Value * scheme_mul(int num_args, Value *args);
Value * scheme_div(int num_args, Value *args);

Value * scheme_cons(int num_args, Value *args);
Value * scheme_car(int num_args, Value *args);
Value * scheme_cdr(int num_args, Value *args);
Value * scheme_list(int num_args, Value *args);
Value * scheme_length(int num_args, Value *args);

Value * scheme_set_car(int num_args, Value *args);
Value * scheme_set_cdr(int num_args, Value *args);

Value * scheme_display(int num_args, Value *args);
Value * scheme_error(int num_args, Value *args);

Value * scheme_srandom(int num_args, Value *args);
Value * scheme_random(int num_args, Value *args);
Value * scheme_time(int num_args, Value *args);

Value * scheme_sqrt(int num_args, Value *args);

Value * scheme_eval_file(int num_args, Value *args);

#endif /* NATIVE_LAMBDAS_H */


