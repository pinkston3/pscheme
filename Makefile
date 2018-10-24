OBJS=ptr_vector.o values.o alloc.o parse.o special_forms.o \
	native_lambdas.o evaluator.o repl.o

CFLAGS=-Wall -Werror

LDFLAGS=-lm


all:  pscheme

pscheme: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o pscheme $(LDFLAGS)

docs:
	doxygen

clean:
	rm -f *.gch *.o *~ pscheme
	rm -rf docs/html

.PHONY: all clean docs
