#ifndef EVALUATOR_H
#define EVALUATOR_H

#include "types.h"


/* Functions for managing environments. */

Environment * init_global_environment(void);
Environment * get_global_environment(void);

Environment * make_environment(Environment *parent_env);

/* Functions for managing name/value bindings in environments. */
int create_binding(Environment *env, char *name, Value *v);
int update_binding(Environment *env, char *name, Value *v);
Value * resolve_binding(Environment *env, char *name);
Value * bind_names_values(Environment *env, Value *names, Value *values);

/*
 * Functions for managing evaluation contexts, which are used for the explicit
 * stack used in evaluation.
 */
PtrStack * get_eval_stack(void);
EvaluationContext * push_new_evalctx(Environment *env, Value *expr);
EvaluationContext * get_current_evalctx(void);
void evalctx_register(Value **v);
EvaluationContext * reset_current_evalctx(Environment *env, Value *expr);
void pop_evalctx(Value *result);

/* The main function that drives expression evaluation. */
Value * evaluate(Environment *env, Value *expr);


#endif /* EVALUATOR_H */

