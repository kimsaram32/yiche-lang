#ifndef YICHE_ERROR_H
#define YICHE_ERROR_H

#include <stdlib.h>
#include "tokenizer.h"

#define exit_with_error(...) do { \
  fprintf(stderr, __VA_ARGS__); \
  exit(EXIT_FAILURE); \
} while (0)

NORETURN void exit_out_of_memory(void);

NORETURN void exit_lexical_error(char *s);
NORETURN void exit_parsing_error_unexpected_token(token_t *token);
NORETURN void exit_parsing_error_no_tokens(void);

#define exit_runtime_error(...) exit_with_error("runtime error: " __VA_ARGS__)

#endif
