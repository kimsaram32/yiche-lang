#include <stdio.h>
#include "yiche.h"

typedef struct
{
  int c, column, line;
} get_char_history_item_t;

#define GET_CHAR_MAX_HISTORY_SIZE 2

static get_char_history_item_t get_char_history[GET_CHAR_MAX_HISTORY_SIZE + 1] = {
  { '\0', 0, 1 } // sentinel
};
static int get_char_history_index = 0;
static int get_char_backs = 0;

#define GET_CHAR_LAST_HISTORY_ITEM (get_char_history[get_char_history_index % (GET_CHAR_MAX_HISTORY_SIZE + 1)])

/* for now, at most two character needs to be buffered. */

int get_char(void)
{
  if (get_char_backs)
  {
    get_char_history_index++;
    get_char_backs--;
    return GET_CHAR_LAST_HISTORY_ITEM.c;
  }

  int c = GET_CHAR_LAST_HISTORY_ITEM.c,
      column = GET_CHAR_LAST_HISTORY_ITEM.column,
      line = GET_CHAR_LAST_HISTORY_ITEM.line;

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

  get_char_history_index++;
  GET_CHAR_LAST_HISTORY_ITEM.c = c;
  GET_CHAR_LAST_HISTORY_ITEM.column = column;
  GET_CHAR_LAST_HISTORY_ITEM.line = line;

  return c;
}

void unget_char(void)
{
  if (get_char_backs == GET_CHAR_MAX_HISTORY_SIZE)
    exit_with_error("unget_char(): maximum count exceeded");

  if (get_char_history_index < 0)
    exit_with_error("unget_char(): no character to unget");

  get_char_history_index--;
  get_char_backs++;
}

