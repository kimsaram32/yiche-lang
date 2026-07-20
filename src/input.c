#include <stdio.h>
#include "yiche.h"

#define INPUT_MAX_HISTORY_SIZE 2

/* for now, at most two character needs to be buffered. */

static input_char_t input_history[INPUT_MAX_HISTORY_SIZE + 1] = {
  { 0, 0, 1 } // sentinel
};
static int input_history_index = 0;
static int input_backs = 0;

#define INPUT_LAST_HISTORY_ITEM (input_history[input_history_index % (INPUT_MAX_HISTORY_SIZE + 1)])

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
}

