#ifndef YICHE_INPUT_H
#define YICHE_INPUT_H

#include <stdio.h>

typedef struct input_context_t input_context_t;

typedef struct
{
  // 0 is used for marking EOF.
  char c;
  int line, column;
}
input_char_t;

// 'stream' must not be used elsewhere until 'input_context_free()' is called.
input_context_t *input_context_create(FILE *stream);

void input_context_free(input_context_t *ctx);

// 2026-07-21 The function 'input_get_last_char' could be removed by making
// 'input_consume_char' and 'input_peek_char' return the full input_char_t
// struct. But most callers only need the raw character, and writing .c
// everywhere would make the code verbose. Therefore, I separated the function for
// retrieving the full information.

input_char_t input_get_last_char(input_context_t *ctx);
char input_consume_char(input_context_t *ctx);
char input_peek_char(input_context_t *ctx, int n);

// the buffer changes as input_consume_char() is called. return the
// accumulated string and clear the buffer.
char *input_get_and_clear_buffer(input_context_t *ctx);

#endif
