#ifndef _ERROR_H
#define _ERROR_H

#include <stdlib.h>

#define exit_with_error(s, ...) do { \
  fprintf(stderr, s, ## __VA_ARGS__); \
  exit(EXIT_FAILURE); \
} while (0)

void exit_out_of_memory(void);

#endif
