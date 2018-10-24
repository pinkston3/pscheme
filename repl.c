/*! \file
 * This file implements the Read-Eval-Print Loop (REPL) for the simple CS24
 * Scheme interpreter.
 */


#include <stdio.h>
#include <assert.h>

#include "alloc.h"
#include "parse.h"
#include "evaluator.h"


/* Change to #define VERBOSE to see garbage-collection debug output. */
#undef VERBOSE


int read_eval_print_loop(FILE *input, const char *prompt, FILE *output) {

    Value *expr, *result;
    Environment *global_env;

    assert(input != NULL);

    global_env = get_global_environment();
    
    while (1) {
        if (prompt != NULL && output != NULL)
            fprintf(output, "%s", prompt);

        expr = read_value(input, 1);
        if (expr == NULL) {
            if (output != NULL)
                fprintf(output, "EOF\n");

            break;
        }

        reset_current_evalctx(global_env, expr);
        result = evaluate(global_env, expr);

        /* If we have interactive style output then we don't terminate the loop
         * on error; we just print the error and then wait for more input.
         * However, if we don't have an output stream, we need to just exit the
         * eval loop.
         */
        if (result != NULL && output != NULL) {
            print_value(output, result);
            fprintf(output, "\n\n");
        }
        else if (is_error(result)) {
            return 0;
        }
        
        /* Make sure every last bit of garbage is gone! */
        reset_current_evalctx(global_env, NULL);
        collect_garbage();
    }

    /* Report success. */
    return 1;
}


int exec_file(const char *filename) {
    FILE *f;
    int result;
    
    f = fopen(filename, "r");
    if (f == NULL) {
        fprintf(stdout, "Couldn't open file \"%s\"!\n", filename);
        return 0;
    }

    result = read_eval_print_loop(f, NULL, NULL);

    fclose(f);
    
    return result;
}


/*!
 * This main function provides a simple Read-Eval-Print Loop (REPL) for the CS24
 * Scheme interpreter.  The first thing it does is to set up the global
 * environment, and the root evaluation context which is always present on the
 * stack so that evaluation can store its results into this root context.
 */
int main() {
    Environment *global_env;
    EvaluationContext *root_eval_ctx;

    init_alloc();
    global_env = init_global_environment();
    root_eval_ctx = push_new_evalctx(NULL, NULL);

    fprintf(stdout, "Loading standard functions...");    
    if (!exec_file("stdlib.scm")) {
        fprintf(stdout, "\nError loading standard functions!  Exiting.\n");
        return 2;
    }
    fprintf(stdout, "  done.\n");
    
#ifdef VERBOSE
    fprintf(stdout, "[Initial] ");
    print_alloc_stats(stdout);
    fprintf(stdout, "\n");
#endif

    read_eval_print_loop(stdin, "> ", stdout);

    return 0;
}

