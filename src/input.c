#include <stdio.h>
#include <string.h>
#include "yiche.h"

#define INPUT_MAX_BUFFER_SIZE 127

static VECTOR_T(input_char_t) *input_buffer = NULL;

static int input_next_index = 0;

static input_char_t file_next_char(void);

void input_init(void)
{
  input_buffer = vector_create(sizeof(input_char_t), 128);
}

input_char_t input_get_last_char(void)
{
  if (input_next_index == 0)
    exit_with_error("input_get_last_char(): no last read character");

  return VECTOR_ARR(input_buffer, input_char_t)[input_next_index - 1];
}

static void append_next_char_from_file(void)
{
  input_char_t *next = vector_next_element(input_buffer);
  if (next == NULL)
    exit_out_of_memory();

  *next = file_next_char();
}

unsigned char input_advance_char(void)
{
  if (input_next_index == input_buffer->length)
    append_next_char_from_file();

  return VECTOR_ARR(input_buffer, input_char_t)[input_next_index++].c;
}

unsigned char input_peek_char(int n)
{
  int peeking = input_next_index + n - 1;

  while (peeking >= input_buffer->length)
    append_next_char_from_file();

  return VECTOR_ARR(input_buffer, input_char_t)[peeking].c;
}

char *input_get_and_clear_buffer(void)
{
  int i;
  char *buf = malloc(sizeof(char) * (input_next_index + 1));
  if (buf == NULL)
    return NULL;

  input_char_t *input_buffer_arr = VECTOR_ARR(input_buffer, input_char_t);

  for (i = 0; i < input_next_index; i++)
    buf[i] = input_buffer_arr[i].c;
  buf[input_next_index] = '\0';

  for (i = 0; input_next_index < input_buffer->length; i++, input_next_index++)
    input_buffer_arr[i] = input_buffer_arr[input_next_index];

  input_next_index = 0;
  input_buffer->length = i;

  return buf;
}

static input_char_t file_last_char = {
  .c = 0, .line = 1, .column = 0, // sentinel
};

static input_char_t file_next_char(void)
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

    c = fgetc(stdin);
  }
  while (c != EOF && !is_character(c));

  file_last_char.c = c == EOF ? 0 : c;

  return file_last_char;
}
