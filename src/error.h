#ifndef YICHE_ERROR_H
#define YICHE_ERROR_H

#include <stdlib.h>
#include "tokenizer.h"

#define exit_with_error(...) do { \
  fprintf(stderr, __VA_ARGS__); \
  exit(EXIT_FAILURE); \
} while (0)

NORETURN void exit_out_of_memory(void);

#define exit_runtime_error(...) exit_with_error("runtime error: " __VA_ARGS__)

#endif
