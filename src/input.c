#include <stdio.h>
#include <string.h>
#include "yiche.h"

typedef struct input_context_t
{
  FILE *stream;
  // The last character read in the stream.
  input_char_t stream_last_char;

  VECTOR_T(input_char_t) *buffer;
  int buffer_next_index;
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
  ctx->stream_last_char = (input_char_t){
    .c = 0, .line = 1, .column = 0, // sentinel
  };

  ctx->buffer_next_index = 0;

  return ctx;
}

void input_context_free(input_context_t *ctx)
{
  vector_free(ctx->buffer);
  free(ctx);
}

input_result_t input_get_last_char(input_context_t *ctx, input_char_t *input_char)
{
  if (ctx->buffer_next_index == 0)
    return INPUT_NO_LAST_READ;

  *input_char = VECTOR_ARR(ctx->buffer, input_char_t)[ctx->buffer_next_index - 1];
  return INPUT_SUCCESS;
}

// Get the next valid character (according to 'is_character()') or an EOF from
// the stream, with the line/column information.
static input_char_t input_next_valid_char_from_stream(input_context_t *ctx)
{
  int c = ctx->stream_last_char.c;

  do
  {
    if (c == '\n')
    {
      ctx->stream_last_char.line++;
      ctx->stream_last_char.column = 1;
    }
    else
      ctx->stream_last_char.column++;

    c = fgetc(ctx->stream);
  }
  while (c != EOF && !is_character(c));

  ctx->stream_last_char.c = c == EOF ? 0 : c;

  return ctx->stream_last_char;
}

input_result_t input_consume_char(input_context_t *ctx, char *c)
{
  if (ctx->buffer_next_index == ctx->buffer->length)
  {
    input_char_t next_char = input_next_valid_char_from_stream(ctx);
    if (!vector_append(ctx->buffer, &next_char))
      return INPUT_OOM;
  }

  if (c != NULL)
    *c = VECTOR_ARR(ctx->buffer, input_char_t)[ctx->buffer_next_index].c;

  ctx->buffer_next_index++;

  return INPUT_SUCCESS;
}

input_result_t input_peek_char(input_context_t *ctx, char *c, int n)
{
  if (n <= 0)
    exit_with_error("input_peek_char(): 'n' must be a positive integer");

  int peeking = ctx->buffer_next_index + n - 1;

  while (peeking >= ctx->buffer->length)
  {
    input_char_t next_char = input_next_valid_char_from_stream(ctx);
    if (!vector_append(ctx->buffer, &next_char))
      return INPUT_OOM;
  }

  *c = VECTOR_ARR(ctx->buffer, input_char_t)[peeking].c;
  return INPUT_SUCCESS;
}

char *input_get_and_clear_buffer(input_context_t *ctx)
{
  int i;
  char *buf = malloc(sizeof(char) * (ctx->buffer_next_index + 1));
  if (buf == NULL)
    return NULL;

  input_char_t *input_buffer_arr = VECTOR_ARR(ctx->buffer, input_char_t);

  for (i = 0; i < ctx->buffer_next_index; i++)
    buf[i] = input_buffer_arr[i].c;
  buf[ctx->buffer_next_index] = '\0';

  for (i = 0; ctx->buffer_next_index < ctx->buffer->length; i++, ctx->buffer_next_index++)
    input_buffer_arr[i] = input_buffer_arr[ctx->buffer_next_index];

  ctx->buffer_next_index = 0;
  ctx->buffer->length = i;

  return buf;
}
