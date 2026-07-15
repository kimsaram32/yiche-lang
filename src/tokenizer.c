#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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

#define GET_CHAR_LAST_HISTORY_ITEM (get_char_history[get_char_history_index % (GET_CHAR_MAX_HISTORY_SIZE + 1)])

static void exit_with_lexical_error(char *s)
{
  exit_with_error("lexical error at line %d, column %d:\n%s",
                  GET_CHAR_LAST_HISTORY_ITEM.line, GET_CHAR_LAST_HISTORY_ITEM.column, s);
}

token_t *tokens;
int tokens_size;
static int tokens_capacity = 64;

static void reallocate_tokens_for_next_if_needed(void)
{
  if (tokens != NULL && tokens_size < tokens_capacity)
    return;

  tokens_capacity *= 2;
  token_t *old = tokens;
  if ((tokens = realloc(tokens, sizeof(token_t) * tokens_capacity)) == NULL)
  {
    free(old);
    exit_out_of_memory();
  }
}

static void token_append_identifier(char *identifier)
{
  reallocate_tokens_for_next_if_needed();
  tokens[tokens_size].type = TOKEN_IDENTIFIER;
  tokens[tokens_size].identifier = identifier;
  tokens_size++;
}

static void token_append_symbol(char *symbol)
{
  reallocate_tokens_for_next_if_needed();
  tokens[tokens_size].type = TOKEN_SYMBOL;
  tokens[tokens_size].symbol = symbol;
  tokens_size++;
}

static void token_append_keyword(token_type_t keyword_type)
{
  reallocate_tokens_for_next_if_needed();
  tokens[tokens_size].type = keyword_type;
  tokens_size++;
}

static void token_append_constant(int constant)
{
  reallocate_tokens_for_next_if_needed();
  tokens[tokens_size].type = TOKEN_CONSTANT;
  tokens[tokens_size].constant = constant;
  tokens_size++;
}

/*
 * Character class
 */

static int is_visible_character(char c)
{
  return c >= 33 && c <= 126;
}

static int is_whitespace(char c)
{
  return c == ' ' || c == '\t' || c == '\n' || c == '\r';
}

static int is_letter(char c)
{
  return (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') || c == '_';
}

static int is_digit(char c)
{
  return c >= '0' && c <= '9';
}

static int is_character(char c)
{
  return is_visible_character(c) || is_whitespace(c);
}

/*
 * Input
 */

static int get_char_backs = 0;
/* for now, at most two character needs to be buffered. */
static int get_char_buffer[2];
static int get_char_buffer_size = 0;

static int get_char(void)
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

static void unget_char()
{
  if (get_char_backs == GET_CHAR_MAX_HISTORY_SIZE)
    exit_with_error("Maximum size exceeded for unget_char() calls");

  if (get_char_history_index < 0)
    exit_with_error("No character to unget");

  get_char_history_index--;
  get_char_backs++;
}

/*
 * Tokenizing
 */

static void skip_multi_line_comment(void)
{
  int c, can_exit = 0;

  while ((c = get_char()) != EOF)
  {
    if (c == '*')
      can_exit = 1;
    else if (c == '/' && can_exit)
      return;
  }

  exit_with_lexical_error("unclosed multi-line comment");
}

static int string_to_keyword(char *s)
{
  if (!strcmp(s, "float"))
    return TOKEN_KEYWORD_FLOAT;
  else if (!strcmp(s, "fn"))
    return TOKEN_KEYWORD_FN;
  else if (!strcmp(s, "if"))
    return TOKEN_KEYWORD_IF;
  else if (!strcmp(s, "int"))
    return TOKEN_KEYWORD_INT;
  else if (!strcmp(s, "return"))
    return TOKEN_KEYWORD_RETURN;
  else if (!strcmp(s, "var"))
    return TOKEN_KEYWORD_VAR;
  else if (!strcmp(s, "while"))
    return TOKEN_KEYWORD_WHILE;
  else
    return -1;
}

static char *read_keyword_or_identifier(char initial)
{
  char *buf;
  int buf_capacity = 8, i = 1;

  if ((buf = malloc(sizeof(char) * buf_capacity)) == NULL)
    return NULL;

  buf[0] = initial;

  while (is_letter((buf[i] = get_char())) || is_digit(buf[i]))
  {
    if (++i == buf_capacity)
    {
      buf_capacity *= 2;
      char *old = buf;
      if ((buf = realloc(buf, sizeof(char) * buf_capacity)) == NULL)
      {
        free(old);
        return NULL;
      }
    }
  }

  unget_char(); // note: the char may be EOF (also applies for other read_ functions)
  buf[i] = '\0';

  return buf;
}

static int valid_symbols_1[128] = {
  [':'] = 1,  [';'] = 1, [','] = 1,
  ['+'] = 1,  ['-'] = 1,  ['*'] = 1,  ['/'] = 1,  ['%'] = 1,
  ['<'] = 1,  ['>'] = 1,
  ['!'] = 1,
  ['='] = 1,
  ['('] = 1,  [')'] = 1,  ['{'] = 1,  ['}'] = 1,
};

static char *valid_symbols_2[] = { "<=", ">=", "==", "!=", "&&", "||", };

static char *read_symbol(char initial)
{
  if (!valid_symbols_1[initial])
    exit_with_lexical_error("invalid symbol");

  char *buf = calloc(3, sizeof(char));
  buf[0] = initial;

  if ((buf[1] = get_char()) != EOF)
  {
    for (int i = 0; i < ARRAY_LENGTH(valid_symbols_2); i++)
      if (!strcmp(buf, valid_symbols_2[i]))
        return buf;

    unget_char();
    buf[1] = '\0';
  }

  return buf;
}

static int is_hexadecimal_digit(char c)
{
  return is_digit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static int read_numeric_constant(char initial)
{
  int val = 0, c, p;

  if (initial >= '1' && initial <= '9') // (1) decimal constant greater than 0
  {
    c = initial;

    do
      val = val * 10 + c - '0';
    while (is_digit((c = get_char())));
  }
  else if ((c = get_char()) == 'x' || c == 'X') // may be a hexadecimal constant
  {
    p = c;

    if (is_hexadecimal_digit((c = get_char()))) // (2) hexadecimal constant
    {
      do
      {
        val *= 16;
        if (c >= 'a')
          val += c - 'a' + 10;
        else if (c >= 'A')
          val += c - 'A' + 10;
        else
          val += c - '0';
      }
      while (is_hexadecimal_digit((c = get_char())));
    }
    else // (3) decimal constant 0
    {
      unget_char();
      c = p;
    }
  }
  // (3) decimal constant 0

  unget_char(); // in all cases, we read one more character
  return val;
}

static int get_escape_character_value(char c)
{
  switch (c)
  {
    case '0':
      return '\0';
    case 'a':
      return '\a';
    case 'b':
      return '\b';
    case 't':
      return '\t';
    case 'n':
      return '\n';
    case 'v':
      return '\v';
    case 'f':
      return '\f';
    case 'r':
      return '\r';
    case 'e':
      return '\e';
    case '\'':
    case '\\':
      return c;
    default:
      return -1;
  }
}

static int read_character_constant(void)
{
  int c = get_char(), val;

  if (c == '\'')
    exit_with_lexical_error("empty character constant '' not allowed");
  else if (c == '\\')
  {
    if ((val = get_escape_character_value(get_char())) == -1)
      exit_with_lexical_error("invalid escape sequence");
  }
  else if (is_visible_character(c) || c == ' ' || c == '\t')
    val = c;
  else
    exit_with_lexical_error("invalid character constant sequence");

  if (get_char() != '\'')
    exit_with_lexical_error("invalid character constant sequence");

  return val;
}

void tokenize(void)
{
  int c, prev;
  while ((c = get_char()) != EOF)
  {
    if (is_whitespace(c))
      continue;

    if (is_letter(c))
    {
      char *keyword_or_identifier = read_keyword_or_identifier(c);
      if (keyword_or_identifier == NULL)
        exit_out_of_memory();

      int keyword_type = string_to_keyword(keyword_or_identifier);
      if (keyword_type == -1)
        token_append_identifier(keyword_or_identifier);
      else
        token_append_keyword(keyword_type);
    }
    else if (is_digit(c))
      token_append_constant(read_numeric_constant(c));
    else if (c == '\'')
      token_append_constant(read_character_constant());
    else if (c == '/')
    {
      prev = c;
      c = get_char();

      if (c == '/')
      {
        while ((c = get_char()) != '\n');
        // technically '\n' is not part of <single_line_comment>, but it'll get
        // skipped anyway, so do not unget_char() it
      }
      else if (c == '*')
        skip_multi_line_comment();
      else
      {
        unget_char();
        token_append_symbol(read_symbol(prev));
      }
    }
    else
      token_append_symbol(read_symbol(c));
  }
}
