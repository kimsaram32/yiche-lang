#ifndef YICHE_ERROR_H
#define YICHE_ERROR_H

#include <stdlib.h>
#include "tokenizer.h"

#define exit_with_error(...) do { \
  fprintf(stderr, __VA_ARGS__); \
  exit(EXIT_FAILURE); \
} while (0)

NORETURN void exit_out_of_memory(void);

#endif
