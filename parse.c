#include "parse.h"
#include "values.h"

#include <assert.h>
#include <ctype.h>
#include <string.h>



static Token curr_token;

static const char *token_types[] = {
    "STREAM_END", "ERROR", "LPAREN", "RPAREN", "SQUOTE", "DQUOTE", "VALUE",
    "STRING_VALUE"
};


/*===========================================*/
/* DECLARATIONS OF INTERNAL HELPER FUNCTIONS */
/*===========================================*/

Value * read_atom_or_number(const Token *p_tok);
Value * read_list(FILE *f);



/*======================*/
/* FUNCTION DEFINITIONS */
/*======================*/

void print_token(const Token *p_tok) {
    if (p_tok->type == VALUE || p_tok->type == STRING_VALUE)
        printf("%s : %s\n", token_types[p_tok->type], p_tok->string);
    else
        printf("%s\n", token_types[p_tok->type]);
}


void print_curr_token(void) {
    print_token(&curr_token);
}


TokenType next_token(FILE *f) {
    int i, ch;

    while (1) {
        /* Consume whitespace. */
        do {
            ch = fgetc(f);
        }
        while (ch != EOF && isspace(ch));

        /* Handle case where we hit EOF while reading. */
        if (ch == EOF) {
            curr_token.type = STREAM_END;
            goto Done;
        }
        
        /* Consume comments. */
        if (ch == ';') {
            do {
                ch = fgetc(f);
            }
            while (ch != EOF && ch != '\r' && ch != '\n');

            /* Handle case where we hit EOF while reading. */
            if (ch == EOF) {
                curr_token.type = STREAM_END;
                goto Done;
            }
        }
        else {
            break;
        }
    }

    /* If we got here, ch absolutely cannot be EOF! */
    assert(ch != EOF);
    
    if (ch == '(') {
        curr_token.type = LPAREN;
    }
    else if (ch == ')') {
        curr_token.type = RPAREN;
    }
    else if (ch == '\'') {
        curr_token.type = SQUOTE;
    }
    else {
        if (ch == '\"') {
            /*
             * Need to parse a double-quoted string.  The token's string-value
             * DOES NOT include the double-quotes.
             */

            curr_token.type = STRING_VALUE;

            for (i = 0; i < MAX_LENGTH; i++) {
                ch = fgetc(f);

                if (ch == EOF) {
                    /* It's an error if we reach end of stream in the middle
                     * of a double-quoted string!
                     */
                    fprintf(stderr, "ERROR:  "
                            "Hit unexpected EOF while parsing a string.\n");
                    curr_token.type = ERROR;
                    break;
                }
                else if (ch == '\r' || ch == '\n') {
                    fprintf(stderr, "ERROR:  "
                            "Strings must be specified on a single line.\n");
                    curr_token.type = ERROR;
                    break;
                }
                else if (ch == '\"') {
                    curr_token.string[i] = 0;  /* Zero-terminate string. */
                    break;
                }

                curr_token.string[i] = ch;
            }
        }
        else {
            /* This is some other kind of non-string value to parse. */

            curr_token.type = VALUE;
            curr_token.string[0] = ch;

            for (i = 1; i < MAX_LENGTH; i++) {
                ch = fgetc(f);
                
                if (ch == EOF) {
                    /* It's probably an error if we hit EOF in this case, but
                     * we'll let the other parsing code handle that case.
                     */
                    curr_token.string[i] = 0;  /* Zero-terminate the string. */
                    break;
                }

                if (isspace(ch) || ch == ')') {
                    /* Hit end of value.  Push char back, then exit. */
                    ungetc(ch, f);
                    curr_token.string[i] = 0;  /* Zero-terminate the string. */
                    break;
                }

                curr_token.string[i] = ch;
            }
            if (i == MAX_LENGTH) {
                fprintf(stderr,
                        "ERROR:  Token was more than %d characters long!\n",
                        MAX_LENGTH);
                curr_token.string[MAX_LENGTH - 1] = '\0';
            }

            /* Distinguish between a numeric value and a simple period. */
            if (strlen(curr_token.string) == 1 && curr_token.string[0] == '.')
                curr_token.type = PERIOD;
        }
    }

Done:
    return curr_token.type;
}


Value * read_value(FILE *f, int advance) {
    Value *val = NULL;

    if (advance)
        next_token(f);

    switch (curr_token.type) {
    case LPAREN:
        /* This value is a (possibly empty) list. */
        val = read_list(f);
        break;

    case VALUE:
        val = read_atom_or_number(&curr_token);
        break;

    case STRING_VALUE:
        val = make_string(curr_token.string);
        break;

    case SQUOTE:   /* Handle the sugared quote syntax. */
        {
            Value *quoted = read_value(f, 1);
            val = make_cons(make_atom("quote"), make_cons(quoted, make_nil()));
        }
        break;

    case STREAM_END:
    case ERROR:
        /* These ones we ignore, and just return a NULL value. */
        break;
        
    default:
        fprintf(stderr, "ERROR:  Unexpected token!");
    }

    return val;
}


Value * read_atom_or_number(const Token *p_tok) {
    Value *val = NULL;
    int str_len;
    int count_parsed, chars_consumed;
    float fval;

    assert(p_tok != NULL);
    assert(p_tok->type == VALUE);

    /* NOTE:  Couldn't get strtof()/strtod() to work... */
    str_len = strlen(p_tok->string);
    count_parsed = sscanf(p_tok->string, "%f%n", &fval, &chars_consumed);
    if (count_parsed == 0) {
        /* No conversion occurred at all.  This value is an atom or a Boolean
         * literal.
         */

        if (strcmp(p_tok->string, "nil") == 0)
            val = make_nil();
        else if (strcmp(p_tok->string, "#t") == 0)
            val = make_bool(1);
        else if (strcmp(p_tok->string, "#f") == 0)
            val = make_bool(0);
        else
            val = make_atom(p_tok->string);
    }
    else if (chars_consumed != str_len) {
        /* Didn't parse the entire value, so assume it's a number with an
         * invalid format.
         */
        fprintf(stderr, "ERROR:  Invalid number format \"%s\".\n",
                p_tok->string);
    }
    else {
        /* This is a number. */
        val = make_float(fval);
    }

    return val;
}



Value * read_list(FILE *f) {
    TokenType tok_type;

    Value *first_cons = NULL;
    Value *last_cons = NULL;

    Value *elem_cons;
    Value *elem_value;

    int length = 0;

    while (1) {
        /*
         * Advance to the next token.  If we have hit the end of the list, go
         * ahead and break out.
         */
        tok_type = next_token(f);
        if (tok_type == RPAREN || tok_type == PERIOD)
            break;

        elem_value = read_value(f, 0);
        length++;

        if (is_error(elem_value))
            return elem_value;

        elem_cons = make_cons(elem_value, NULL);
        if (last_cons != NULL)
            set_cdr(last_cons, elem_cons);
        else
            first_cons = elem_cons;

        last_cons = elem_cons;
    }

    if (tok_type == PERIOD && length == 0) {
        return make_error("Invalid list:  "
                          "Period must be preceded by at least one value.");
    }

    if (length == 0) {
        /* Result is nil, not a list. */
        assert(tok_type == RPAREN);
        return make_nil();
    }
    else {
        /*
         * Result is a list, but the last cons-pair still has NULL for its cdr.
         */

        if (tok_type == RPAREN) {
            /* Easy case:  Just set cdr = nil. */
            set_cdr(last_cons, make_nil());
        }
        else {
            /* Last value is supposed to be a dotted-pair.  Read final value. */
            elem_value = read_value(f, 1);
            if (is_error(elem_value))
                return elem_value;

            tok_type = next_token(f);
            if (tok_type != RPAREN) {
                return make_error("Invalid list:  "
                                  "Only one value may follow period.");
            }

            set_cdr(last_cons, elem_value);
        }
    }

    return first_cons;
}

