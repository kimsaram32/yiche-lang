#include <stdio.h>
#include <stdlib.h>
#include "yiche.h"

NORETURN void exit_out_of_memory(void)
{
  exit_with_error("out of memory");
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
