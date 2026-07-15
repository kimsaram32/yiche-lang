#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include <stdio.h>

/*
 * Token type
 */

typedef enum
{
  TOKEN_KEYWORD_FLOAT,
  TOKEN_KEYWORD_FN,
  TOKEN_KEYWORD_IF,
  TOKEN_KEYWORD_INT,
  TOKEN_KEYWORD_RETURN,
  TOKEN_KEYWORD_VAR,
  TOKEN_KEYWORD_WHILE,
  TOKEN_IDENTIFIER,
  TOKEN_SYMBOL,
  TOKEN_CONSTANT,
}
token_type_t;

typedef struct
{
  union
  {
    char *identifier, *symbol;
  };
  int constant;
  token_type_t type;
}
token_t;

extern token_t *tokens;
extern int tokens_size;

void tokenize(void);

#endif
