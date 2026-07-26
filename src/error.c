#include <stdio.h>
#include <stdlib.h>
#include "yiche.h"

NORETURN void exit_out_of_memory(void)
{
  exit_with_error("out of memory");
}

NORETURN void exit_lexical_error(char *s)
{
  input_char_t last_char = input_get_last_char();
  exit_with_error("lexical error at line %d, column %d ('%c'):\n%s",
                  last_char.line, last_char.column, last_char.c, s);
}

NORETURN void exit_parsing_error_unexpected_token(token_t *token)
{
  exit_with_error("syntax error at line %d: unexpected token '%s'\n",
                  token->char_begin.line, token->lexeme);
}

NORETURN void exit_parsing_error_no_tokens(void)
{
  exit_with_error("syntax error: expected a token, got nothing\n");
}
