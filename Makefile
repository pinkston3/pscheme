OBJS=ptr_vector.o values.o alloc.o parse.o special_forms.o \
	native_lambdas.o evaluator.o repl.o

CFLAGS=-Wall -Werror

LDFLAGS=-lm


all:  scheme24

scheme24: $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o scheme24 $(LDFLAGS)

docs:
	doxygen

clean:
	rm -f *.gch *.o *~ scheme24
	rm -rf docs/html

.PHONY: all clean docs
