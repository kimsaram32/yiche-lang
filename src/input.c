#include <stdio.h>
#include <string.h>
#include "yiche.h"

#define INPUT_MAX_BUFFER_SIZE 127

static input_char_t input_buffer[INPUT_MAX_BUFFER_SIZE];

static int input_next_index = 0;
static int input_next_read_index = 0;

static input_char_t file_next_char(void);

input_char_t input_get_last_char(void)
{
  if (input_next_index == 0)
    exit_with_error("input_get_last_char(): no last read character");

  return input_buffer[input_next_index - 1];
}

unsigned char input_advance_char(void)
{
  if (input_next_index == INPUT_MAX_BUFFER_SIZE)
    exit_with_error("input_advance_char(): maximum size for input buffer exceeded");

  if (input_next_index == input_next_read_index)
    input_buffer[input_next_read_index++] = file_next_char();

  return input_buffer[input_next_index++].c;
}

unsigned char input_peek_char(int n)
{
  int peeking = input_next_index + n - 1;

  if (peeking >= INPUT_MAX_BUFFER_SIZE)
    exit_with_error("input_pepek_char(): maximum size for input buffer exceeded");

  while (peeking >= input_next_read_index)
    input_buffer[input_next_read_index++] = file_next_char();

  return input_buffer[peeking].c;
}

char *input_get_and_clear_buffer(void)
{
  int i;
  char *buf = malloc(sizeof(char) * (input_next_index + 1));
  if (buf == NULL)
    return NULL;

  for (i = 0; i < input_next_index; i++)
    buf[i] = input_buffer[i].c;
  buf[input_next_index] = '\0';

  for (i = 0; input_next_index < input_next_read_index; i++, input_next_index++)
    input_buffer[i] = input_buffer[input_next_index];

  input_next_index = 0;
  input_next_read_index = i;

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
