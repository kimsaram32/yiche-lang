#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "yiche.h"

static void exit_with_lexical_error(char *s)
{
  input_char_t last_char = input_get_last_char();
  exit_with_error("lexical error at line %d, column %d ('%c'):\n%s",
                  last_char.line, last_char.column, last_char.c, s);
}

VECTOR_T(token_t) *tokens;

/*
 * Tokenizing
 */

static void skip_multi_line_comment(void)
{
  int c, can_exit = 0;

  while ((c = input_advance_char()) != 0)
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

// TODO: Isn't the buffer a duplication of lexeme?
static char *read_keyword_or_identifier(char initial)
{
  char *buf;
  int buf_capacity = 8, i = 1;

  if ((buf = malloc(sizeof(char) * buf_capacity)) == NULL)
    return NULL;

  buf[0] = initial;

  while (is_letter((buf[i] = input_peek_char(1))) || is_digit(buf[i]))
  {
    input_advance_char();
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

  buf[i] = '\0';

  return buf;
}

// returns a symbol token_type_t or -1.
static int get_symbol_1(char sym)
{
  switch (sym)
  {
    case '(':
      return TOKEN_SYMBOL_LPAREN;
    case ')':
      return TOKEN_SYMBOL_RPAREN;
    case '[':
      return TOKEN_SYMBOL_LBRACKET;
    case ']':
      return TOKEN_SYMBOL_RBRACKET;
    case '{':
      return TOKEN_SYMBOL_LBRACE;
    case '}':
      return TOKEN_SYMBOL_RBRACE;
    case '<':
      return TOKEN_SYMBOL_LT;
    case '>':
      return TOKEN_SYMBOL_GT;
    case '!':
      return TOKEN_SYMBOL_BANG;
    case '%':
      return TOKEN_SYMBOL_PERCENT;
    case '*':
      return TOKEN_SYMBOL_ASTERISK;
    case '+':
      return TOKEN_SYMBOL_PLUS;
    case '-':
      return TOKEN_SYMBOL_MINUS;
    case ':':
      return TOKEN_SYMBOL_COLON;
    case ';':
      return TOKEN_SYMBOL_SEMICOLON;
    case '/':
      return TOKEN_SYMBOL_SLASH;
    case ',':
      return TOKEN_SYMBOL_COMMA;
    case '=':
      return TOKEN_SYMBOL_EQ;
    default:
      return -1;
  }
}

// returns a symbol token type or -1.
static int get_symbol_2(char *sym)
{
  if (!strcmp(sym, "=="))
    return TOKEN_SYMBOL_EQEQ;
  else if (!strcmp(sym, "!="))
    return TOKEN_SYMBOL_BANGEQ;
  else if (!strcmp(sym, "<="))
    return TOKEN_SYMBOL_LE;
  else if (!strcmp(sym, ">="))
    return TOKEN_SYMBOL_GE;
  else if (!strcmp(sym, "&&"))
    return TOKEN_SYMBOL_ANDAND;
  else if (!strcmp(sym, "||"))
    return TOKEN_SYMBOL_OROR;
  else
    return -1;
}

// returns a symbol type.
static token_type_t read_symbol(char initial)
{
  int type;

  if ((type = get_symbol_1(initial)) == -1)
    exit_with_lexical_error("invalid symbol");

  char *buf = calloc(3, sizeof(char));
  buf[0] = initial;

  if ((buf[1] = input_peek_char(1)) != 0)
  {
    int t = get_symbol_2(buf);
    if (t != -1)
    {
      input_advance_char();
      return t;
    }
  }

  return type;
}

static int is_hexadecimal_digit(char c)
{
  return is_digit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static int read_numeric_constant(char initial)
{
  int val = 0, c;

  if (initial >= '1' && initial <= '9')
  {
    // decimal constant greater than 0
    val = val * 10 + initial - '0';

    while (is_digit((c = input_peek_char(1))))
    {
      input_advance_char();
      val = val * 10 + c - '0';
    }
  }
  else if (((c = input_peek_char(1)) == 'x' || c == 'X') && is_hexadecimal_digit(input_peek_char(2)))
  {
    // hexadecimal constant
    input_advance_char();
    while (is_hexadecimal_digit((c = input_peek_char(1))))
    {
      input_advance_char();

      val *= 16;
      if (c >= 'a')
        val += c - 'a' + 10;
      else if (c >= 'A')
        val += c - 'A' + 10;
      else
        val += c - '0';
    }
  } // else: decimal constant zero

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
  int c = input_advance_char(), val;

  if (c == '\'')
    exit_with_lexical_error("empty character constant '' not allowed");
  else if (c == '\\')
  {
    if ((val = get_escape_character_value(input_advance_char())) == -1)
      exit_with_lexical_error("invalid escape sequence");
  }
  else if (is_visible_character(c) || c == ' ' || c == '\t')
    val = c;
  else
    exit_with_lexical_error("invalid character constant sequence");

  if (input_advance_char() != '\'')
    exit_with_lexical_error("invalid character constant sequence");

  return val;
}

void tokenize(void)
{
  if ((tokens = vector_create(sizeof(token_t), 64)) == NULL)
    exit_out_of_memory();

  int c, c2;
  while ((c = input_advance_char()) != 0)
  {
    if (is_whitespace(c))
      goto skip;

    if (c == '/')
    {
      c2 = input_peek_char(1);

      if (c2 == '/')
      {
        // technically '\n' is not part of <single_line_comment>, but it'll get
        // skipped anyway, so do not input_unget_char() it
        while ((c = input_advance_char()) != 0 && c != '\n');

        goto skip;
      }
      else if (c2 == '*')
      {
        input_advance_char();
        skip_multi_line_comment();
        goto skip;
      }
    }

    token_t *token = vector_next_element(tokens);
    if (token == NULL)
      exit_out_of_memory();
    token->char_begin = input_get_last_char();

    if (is_letter(c))
    {
      char *keyword_or_identifier = read_keyword_or_identifier(c);
      if (keyword_or_identifier == NULL)
        exit_out_of_memory();

      int keyword_type = string_to_keyword(keyword_or_identifier);
      if (keyword_type == -1)
      {
        token->type = TOKEN_IDENTIFIER;
        token->identifier = keyword_or_identifier;
      }
      else
        token->type = keyword_type;
    }
    else if (is_digit(c))
    {
      token->type = TOKEN_CONSTANT;
      token->constant = read_numeric_constant(c);
    }
    else if (c == '\'')
    {
      token->type = TOKEN_CONSTANT;
      token->constant = read_character_constant();
    }
    else
      token->type = read_symbol(c);

    token->char_end = input_get_last_char();

    if ((token->lexeme = input_get_and_clear_buffer()) == NULL)
      exit_out_of_memory();
    continue;

    skip: free(input_get_and_clear_buffer());
  }
}

/*
 * Consumption
 */

static int token_next_pos = 0;

token_t *token_advance(void)
{
  token_t *tokens_arr = VECTOR_ARR(tokens, token_t);
  if (token_next_pos == tokens->length)
    return NULL;
  return &tokens_arr[token_next_pos++];
}

token_t *token_peek_next(void)
{
  token_t *tokens_arr = VECTOR_ARR(tokens, token_t);
  if (token_next_pos == tokens->length)
    return NULL;
  return &tokens_arr[token_next_pos];
}

void token_unget(void)
{
  if (token_next_pos == 0)
    exit_with_error("token_unget(): no token to unget");
  token_next_pos--;
}

token_t *token_advance_and_assert(int n, ...)
{
  va_list types;
  token_t *token = token_advance();
  if (token == NULL)
    exit_with_error("expected a token, got nothing\n");

  for (va_start(types, n); n--;)
  {
    if (token->type == va_arg(types, token_type_t))
    {
      va_end(types);
      return token;
    }
  }

  va_end(types);
  exit_with_error("syntax error at line %d: unexpected token '%s'\n",
                  token->char_begin.line, token->lexeme);
}

token_t *token_try_advancing(int n, ...)
{
  va_list types;
  token_t *token = token_peek_next();
  if (token == NULL)
    return NULL;

  for (va_start(types, n); n--;)
  {
    if (token->type == va_arg(types, token_type_t))
    {
      va_end(types);
      return token_advance();
    }
  }

  va_end(types);
  return NULL;
}
