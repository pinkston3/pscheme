#ifndef ALLOC_H
#define ALLOC_H

#include <stdio.h>
#include "values.h"
#include "evaluator.h"

void init_alloc(void);

Value * alloc_value(void);
Lambda * alloc_lambda(void);
Environment * alloc_environment(void);

void collect_garbage(void);

void print_alloc_stats(FILE *f);


#endif /* ALLOC_H */

