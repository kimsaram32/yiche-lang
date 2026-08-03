#include <stdio.h>
#include <string.h>
#include "yiche.h"

typedef struct input_context_t
{
  FILE *stream;
  VECTOR_T(input_char_t) *buffer;
  int next_index;
}
input_context_t;

static input_char_t file_next_char(FILE *stream);

input_context_t *input_context_create(FILE *stream)
{
  input_context_t *ctx = malloc(sizeof(input_context_t));
  if (ctx == NULL)
    return NULL;

  ctx->buffer = vector_create(sizeof(input_char_t), 128, destructor_noop);
  if (ctx->buffer == NULL)
  {
    free(ctx);
    return NULL;
  }

  ctx->stream = stream;
  ctx->next_index = 0;

  return ctx;
}

void input_context_free(input_context_t *ctx)
{
  vector_free(ctx->buffer);
  free(ctx);
}

input_char_t input_get_last_char(input_context_t *ctx)
{
  if (ctx->next_index == 0)
    exit_with_error("input_get_last_char(): no last read character");

  return VECTOR_ARR(ctx->buffer, input_char_t)[ctx->next_index - 1];
}

static void append_next_char_from_file(input_context_t *ctx)
{
  input_char_t next = file_next_char(ctx->stream);

  if (!vector_append(ctx->buffer, &next))
    exit_out_of_memory();
}

char input_consume_char(input_context_t *ctx)
{
  if (ctx->next_index == ctx->buffer->length)
    append_next_char_from_file(ctx);

  return VECTOR_ARR(ctx->buffer, input_char_t)[ctx->next_index++].c;
}

char input_peek_char(input_context_t *ctx, int n)
{
  if (n <= 0)
    exit_with_error("input_peek_char(): 'n' must be a positive integer");

  int peeking = ctx->next_index + n - 1;

  while (peeking >= ctx->buffer->length)
    append_next_char_from_file(ctx);

  return VECTOR_ARR(ctx->buffer, input_char_t)[peeking].c;
}

char *input_get_and_clear_buffer(input_context_t *ctx)
{
  int i;
  char *buf = malloc(sizeof(char) * (ctx->next_index + 1));
  if (buf == NULL)
    return NULL;

  input_char_t *input_buffer_arr = VECTOR_ARR(ctx->buffer, input_char_t);

  for (i = 0; i < ctx->next_index; i++)
    buf[i] = input_buffer_arr[i].c;
  buf[ctx->next_index] = '\0';

  for (i = 0; ctx->next_index < ctx->buffer->length; i++, ctx->next_index++)
    input_buffer_arr[i] = input_buffer_arr[ctx->next_index];

  ctx->next_index = 0;
  ctx->buffer->length = i;

  return buf;
}

static input_char_t file_last_char = {
  .c = 0, .line = 1, .column = 0, // sentinel
};

static input_char_t file_next_char(FILE *stream)
{
  int c = file_last_char.c;

  do
  {
    if (c == '\n')
    {
      file_last_char.line++;
      file_last_char.column = 1;
    }
    else
      file_last_char.column++;

    c = fgetc(stream);
  }
  while (c != EOF && !is_character(c));

  file_last_char.c = c == EOF ? 0 : c;

  return file_last_char;
}
