#ifndef YICHE_TOKENIZER_H
#define YICHE_TOKENIZER_H

#include <stdio.h>
#include "input.h"
#include "vector.h"

/*
 * Token type
 */

typedef enum
{
  TOKEN_IDENTIFIER,
  TOKEN_CONSTANT,

  // Keywords
  TOKEN_KEYWORD_START,
  TOKEN_KEYWORD_FN,
  TOKEN_KEYWORD_IF,
  TOKEN_KEYWORD_INT,
  TOKEN_KEYWORD_RETURN,
  TOKEN_KEYWORD_VAR,
  TOKEN_KEYWORD_WHILE,
  TOKEN_KEYWORD_END,

  // Symbols
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
  TOKEN_SYMBOL_BANG,
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
  TOKEN_SYMBOL_BANGEQ, // !=
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
  token_type_t type;

  char *identifier;
  int constant;

  input_char_t char_begin, char_end;
  char *lexeme;
}
token_t;

// Initializes an array of tokens, reading characters from the standard input.
void tokenize(void);

// The remaining functions operate on the last array of tokens produced by
// 'tokenize()', and must be used after calling it.

void tokens_print(void);

token_t *token_advance(void);
token_t *token_peek(int n);
void token_unget(void);

// If 'token' is non-NULL and matches one of the type(s), return the token.
// Otherwise, throw an error.

token_t *token_assert(token_t *token, token_type_t t1);
token_t *token_assert2(token_t *token, token_type_t t1, token_type_t t2);
token_t *token_assert3(token_t *token, token_type_t t1, token_type_t t2,
                       token_type_t t3);
token_t *token_assert4(token_t *token, token_type_t t1, token_type_t t2,
                       token_type_t t3, token_type_t t4);

// If 'token' is non-NULL and matches one of the type(s), return the token.
// otherwise, return NULL.

token_t *token_check(token_t *token, token_type_t t1);
token_t *token_check2(token_t *token, token_type_t t1, token_type_t t2);
token_t *token_check3(token_t *token, token_type_t t1, token_type_t t2,
                      token_type_t t3);
token_t *token_check4(token_t *token, token_type_t t1, token_type_t t2,
                      token_type_t t3, token_type_t t4);

#endif
