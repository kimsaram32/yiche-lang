#include <stdio.h>
#include <string.h>
#include "yiche.h"

// TODO: Read/history buffers seem to be duplicated.

#define INPUT_MAX_HISTORY_SIZE 2

// For now, at most two character needs to be buffered.

static input_char_t input_history[INPUT_MAX_HISTORY_SIZE + 1] = {
  { 0, 0, 1 } // sentinel
};
static int input_history_index = 0;
static int input_backs = 0;

#define INPUT_LAST_HISTORY_ITEM (input_history[input_history_index % (INPUT_MAX_HISTORY_SIZE + 1)])

#define INPUT_MAX_READ_BUFFER_SIZE 127

static char input_read_buffer[INPUT_MAX_READ_BUFFER_SIZE];
static int input_read_buffer_index = 0;

input_char_t get_last_read_char(void)
{
  return INPUT_LAST_HISTORY_ITEM;
}

unsigned char get_char(void)
{
  if (input_backs)
  {
    input_history_index++;
    input_backs--;
    return INPUT_LAST_HISTORY_ITEM.c;
  }

  int c = INPUT_LAST_HISTORY_ITEM.c,
      column = INPUT_LAST_HISTORY_ITEM.column,
      line = INPUT_LAST_HISTORY_ITEM.line;

  do
  {
    if (c == '\n')
    {
      line++;
      column = 1;
    }
    else
      column++;

    c = fgetc(stdin);
  }
  while (c != EOF && !is_character(c));

  c = c == EOF ? 0 : c;

  input_history_index++;
  INPUT_LAST_HISTORY_ITEM.c = c;
  INPUT_LAST_HISTORY_ITEM.column = column;
  INPUT_LAST_HISTORY_ITEM.line = line;

  // input buffer

  if (input_read_buffer_index == INPUT_MAX_READ_BUFFER_SIZE)
    exit_with_error("get_char(): maximum size for read buffer exceeded");

  input_read_buffer[input_read_buffer_index++] = c;

  return c;
}

void unget_char(void)
{
  if (input_backs == INPUT_MAX_HISTORY_SIZE)
    exit_with_error("unget_char(): maximum count exceeded");

  if (input_history_index < 0)
    exit_with_error("unget_char(): no character to unget");

  input_history_index--;
  input_backs++;

  if (input_read_buffer_index > 0)
    input_read_buffer_index--;
}

char *get_and_clear_read_buffer(void)
{
  char *buf = malloc(sizeof(char) * (input_read_buffer_index + 1));
  if (buf == NULL)
    return NULL;

  strncpy(buf, input_read_buffer, input_read_buffer_index);
  buf[input_read_buffer_index] = '\0';

  input_read_buffer_index = 0;
  return buf;
}
