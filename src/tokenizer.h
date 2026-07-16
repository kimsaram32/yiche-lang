#ifndef _TOKENIZER_H
#define _TOKENIZER_H

#include <stdio.h>

/*
 * Token type
 */

typedef enum
{
  TOKEN_IDENTIFIER,
  TOKEN_CONSTANT,

  // keywords
  TOKEN_KEYWORD_START,
  TOKEN_KEYWORD_FLOAT,
  TOKEN_KEYWORD_FN,
  TOKEN_KEYWORD_IF,
  TOKEN_KEYWORD_INT,
  TOKEN_KEYWORD_RETURN,
  TOKEN_KEYWORD_VAR,
  TOKEN_KEYWORD_WHILE,
  TOKEN_KEYWORD_END,

  // symbols
  // adding a new symbol:
  // - main.c: change symbol_to_string
  // - tokenizer.c: change either get_symbol_1 or get_symbol_2
  TOKEN_SYMBOL_START,
  TOKEN_SYMBOL_LPAREN, // (
  TOKEN_SYMBOL_RPAREN, // )
  TOKEN_SYMBOL_LBRACKET, // [
  TOKEN_SYMBOL_RBRACKET, // ]
  TOKEN_SYMBOL_LBRACE, // {
  TOKEN_SYMBOL_RBRACE, // }
  TOKEN_SYMBOL_LT, // <
  TOKEN_SYMBOL_GT, // >
  TOKEN_SYMBOL_EXCLAMATION,
  TOKEN_SYMBOL_PERCENT,
  TOKEN_SYMBOL_ASTERISK,
  TOKEN_SYMBOL_PLUS,
  TOKEN_SYMBOL_MINUS,
  TOKEN_SYMBOL_COLON,
  TOKEN_SYMBOL_SEMICOLON,
  TOKEN_SYMBOL_SLASH,
  TOKEN_SYMBOL_COMMA,
  TOKEN_SYMBOL_EQ,
  TOKEN_SYMBOL_EQEQ, // ==
  TOKEN_SYMBOL_LE, // <=
  TOKEN_SYMBOL_GE, // >=
  TOKEN_SYMBOL_ANDAND, // &&
  TOKEN_SYMBOL_OROR, // ||
  TOKEN_SYMBOL_END,
}
token_type_t;

#define IS_KEYWORD(token) ((token).type > TOKEN_KEYWORD_START && (token).type < TOKEN_KEYWORD_END)
#define IS_SYMBOL(token) ((token).type > TOKEN_SYMBOL_START && (token).type < TOKEN_SYMBOL_END)

typedef struct
{
  char *identifier;
  int constant;
  token_type_t type;
}
token_t;

extern token_t *tokens;
extern int tokens_size;

void tokenize(void);

#endif
