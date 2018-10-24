#ifndef PARSE_H
#define PARSE_H

#include <stdio.h>
#include "values.h"


/*!
 * This is an enumeration of the various types of tokens that the parser can
 * generate.
 */
typedef enum TokenType {
    STREAM_END,   /*!< Hit EOF while trying to parse. */
    ERROR,        /*!< Ran into a parsing error. */
    LPAREN,       /*!< Left parenthesis. */
    RPAREN,       /*!< Right parenthesis. */
    PERIOD,       /*!< Period. */
    SQUOTE,       /*!< Single-quote. */
    DQUOTE,       /*!< Double-quote. */
    VALUE,        /*!< An atom, a number, or a Boolean, but not a string literal. */
    STRING_VALUE  /*!< A double-quoted string literal. */
} TokenType;


/*!
 * Maximum length of a single token or value.  Specifically, string constants
 * cannot be longer than this length, including their double-quotes, and the
 * terminating NUL character.
 */
#define MAX_LENGTH 200


/*!
 * This struct represents a token identified by the Scheme parser, including its
 * type and the actual string value that was parsed.
 */
typedef struct Token {
    TokenType type;             /*!< The type of the token. */
    char string[MAX_LENGTH];    /*!< The token's actual text. */
} Token;


/* Functions for parsing the Scheme input. */

void print_token(const Token *p_tok);
void print_curr_token(void);

TokenType next_token(FILE *f);

Value * read_value(FILE *f, int advance);


#endif /* PARSE_H */

