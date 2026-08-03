#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "yiche.h"

static VECTOR_T(token_t) *tokens = NULL;

/*
 * Tokenizing
 */

static void skip_multi_line_comment(input_context_t *ctx)
{
  char c;
  int can_exit = 0;

  while ((c = input_consume_char(ctx)) != 0)
  {
    if (c == '*')
      can_exit = 1;
    else if (c == '/' && can_exit)
      return;
  }

  exit_lexical_error(ctx, "unclosed multi-line comment");
}

static int string_to_keyword(char *s)
{
  if (!strcmp(s, "fn"))
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
static char *read_keyword_or_identifier(input_context_t *ctx, char initial)
{
  char *buf;
  int buf_capacity = 8, i = 1;

  if ((buf = malloc(sizeof(char) * buf_capacity)) == NULL)
    return NULL;

  buf[0] = initial;

  while (is_letter((buf[i] = input_peek_char(ctx, 1))) || is_digit(buf[i]))
  {
    input_consume_char(ctx);
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
static int get_symbol_1(char c)
{
  switch (c)
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

// returns a symbol token_type_t or -1.
static int get_symbol_2(char c1, char c2)
{
  if (c1 == '=' && c2 == '=')
    return TOKEN_SYMBOL_EQEQ;
  else if (c1 == '!' && c2 == '=')
    return TOKEN_SYMBOL_BANGEQ;
  else if (c1 == '<' && c2 == '=')
    return TOKEN_SYMBOL_LE;
  else if (c1 == '>' && c2 == '=')
    return TOKEN_SYMBOL_GE;
  else if (c1 == '&' && c2 == '&')
    return TOKEN_SYMBOL_ANDAND;
  else if (c1 == '|' && c2 == '|')
    return TOKEN_SYMBOL_OROR;
  else
    return -1;
}

// returns a symbol type.
static token_type_t read_symbol(input_context_t *ctx, char c1)
{
  char c2;
  int type;

  if ((c2 = input_peek_char(ctx, 1)) != 0)
  {
    type = get_symbol_2(c1, c2);
    if (type != -1)
    {
      input_consume_char(ctx);
      return type;
    }
  }

  if ((type = get_symbol_1(c1)) == -1)
    exit_lexical_error(ctx, "invalid symbol");

  return type;
}

static int is_hexadecimal_digit(char c)
{
  return is_digit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static int read_numeric_constant(input_context_t *ctx, char initial)
{
  char c;
  int val = 0;

  if (initial >= '1' && initial <= '9')
  {
    // decimal constant greater than 0
    val = val * 10 + initial - '0';

    while (is_digit((c = input_peek_char(ctx, 1))))
    {
      input_consume_char(ctx);
      val = val * 10 + c - '0';
    }
  }
  else if (((c = input_peek_char(ctx, 1)) == 'x' || c == 'X') && is_hexadecimal_digit(input_peek_char(ctx, 2)))
  {
    // hexadecimal constant
    input_consume_char(ctx);
    while (is_hexadecimal_digit((c = input_peek_char(ctx, 1))))
    {
      input_consume_char(ctx);

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
      return '\x07'; // BEL
    case 'b':
      return '\x08'; // BS
    case 't':
      return '\x09'; // HT
    case 'n':
      return '\x0A'; // LF
    case 'v':
      return '\x0B'; // VT
    case 'f':
      return '\x0C'; // FF
    case 'r':
      return '\x0D'; // CR
    case 'e':
      return '\x1B'; // ESC
    case '\'':
    case '\\':
      return c;
    default:
      return -1;
  }
}

static int read_character_constant(input_context_t *ctx)
{
  char c = input_consume_char(ctx);
  int val;

  if (c == '\'')
    exit_lexical_error(ctx, "empty character constant '' not allowed");
  else if (c == '\\')
  {
    if ((val = get_escape_character_value(input_consume_char(ctx))) == -1)
      exit_lexical_error(ctx, "invalid escape sequence");
  }
  else if (is_visible_character(c) || c == ' ' || c == '\t')
    val = c;
  else
    exit_lexical_error(ctx, "invalid character constant sequence");

  if (input_consume_char(ctx) != '\'')
    exit_lexical_error(ctx, "invalid character constant sequence");

  return val;
}

static void vector_token_free(void *p)
{
  token_t *token = p;

  free(token->identifier);
  free(token->lexeme);
}

void tokenize(FILE *stream)
{
  input_context_t *ctx = input_context_create(stream);

  if ((tokens = vector_create(sizeof(token_t), 64, vector_token_free)) == NULL)
    exit_out_of_memory();

  char c, c2;
  while ((c = input_consume_char(ctx)) != 0)
  {
    if (is_whitespace(c))
      goto skip;

    if (c == '/')
    {
      c2 = input_peek_char(ctx, 1);

      if (c2 == '/')
      {
        // technically '\n' is not part of <single_line_comment>, but it'll get
        // skipped anyway, so do not input_unget_char(ctx) it
        while ((c = input_consume_char(ctx)) != 0 && c != '\n');

        goto skip;
      }
      else if (c2 == '*')
      {
        input_consume_char(ctx);
        skip_multi_line_comment(ctx);
        goto skip;
      }
    }

    token_t token;
    token.char_begin = input_get_last_char(ctx);

    if (is_letter(c))
    {
      char *keyword_or_identifier = read_keyword_or_identifier(ctx, c);
      if (keyword_or_identifier == NULL)
        exit_out_of_memory();

      int keyword_type = string_to_keyword(keyword_or_identifier);
      if (keyword_type == -1)
      {
        token.type = TOKEN_IDENTIFIER;
        token.identifier = keyword_or_identifier;
      }
      else
      {
        free(keyword_or_identifier);
        token.type = keyword_type;
      }
    }
    else if (is_digit(c))
    {
      token.type = TOKEN_CONSTANT;
      token.constant = read_numeric_constant(ctx, c);
    }
    else if (c == '\'')
    {
      token.type = TOKEN_CONSTANT;
      token.constant = read_character_constant(ctx);
    }
    else
      token.type = read_symbol(ctx, c);

    token.char_end = input_get_last_char(ctx);

    if ((token.lexeme = input_get_and_clear_buffer(ctx)) == NULL)
      exit_out_of_memory();

    if (!vector_append(tokens, &token))
      exit_out_of_memory();

    continue;

    skip: free(input_get_and_clear_buffer(ctx));
  }

  input_context_free(ctx);
}

/*
 * Print representation
 */

static char *keyword_to_string(token_type_t keyword_type)
{
  switch (keyword_type)
  {
    case TOKEN_KEYWORD_FN:
      return "fn";
    case TOKEN_KEYWORD_IF:
      return "if";
    case TOKEN_KEYWORD_INT:
      return "int";
    case TOKEN_KEYWORD_RETURN:
      return "return";
    case TOKEN_KEYWORD_VAR:
      return "var";
    case TOKEN_KEYWORD_WHILE:
      return "while";
    default:
      return NULL;
  }
}

static char *symbol_to_string(token_type_t symbol_type)
{
  switch (symbol_type)
  {
    case TOKEN_SYMBOL_LPAREN:
      return "(";
    case TOKEN_SYMBOL_RPAREN:
      return ")";
    case TOKEN_SYMBOL_LBRACKET:
      return "[";
    case TOKEN_SYMBOL_RBRACKET:
      return "]";
    case TOKEN_SYMBOL_LBRACE:
      return "{";
    case TOKEN_SYMBOL_RBRACE:
      return "}";
    case TOKEN_SYMBOL_LT:
      return "<";
    case TOKEN_SYMBOL_GT:
      return ">";
    case TOKEN_SYMBOL_BANG:
      return "!";
    case TOKEN_SYMBOL_PERCENT:
      return "%";
    case TOKEN_SYMBOL_ASTERISK:
      return "*";
    case TOKEN_SYMBOL_PLUS:
      return "+";
    case TOKEN_SYMBOL_MINUS:
      return "-";
    case TOKEN_SYMBOL_COLON:
      return ":";
    case TOKEN_SYMBOL_SEMICOLON:
      return ";";
    case TOKEN_SYMBOL_SLASH:
      return "/";
    case TOKEN_SYMBOL_COMMA:
      return ",";
    case TOKEN_SYMBOL_EQ:
      return "=";
    case TOKEN_SYMBOL_EQEQ:
      return "==";
    case TOKEN_SYMBOL_BANGEQ:
      return "!=";
    case TOKEN_SYMBOL_LE:
      return "<=";
    case TOKEN_SYMBOL_GE:
      return ">=";
    case TOKEN_SYMBOL_ANDAND:
      return "&&";
    case TOKEN_SYMBOL_OROR:
      return "||";
    default:
      return NULL;
  }
}

void tokens_print(void)
{
  token_t *tokens_arr = VECTOR_ARR(tokens, token_t);

  for (int i = 0; i < tokens->length; i++)
  {
    token_t token = tokens_arr[i];
    if (token.type == TOKEN_IDENTIFIER)
      printf("identifier: %s\n", token.identifier);
    else if (token.type == TOKEN_CONSTANT)
      printf("constant: %d (%s)\n", token.constant, token.lexeme);
    else if (IS_KEYWORD(token))
      printf("keyword: %s\n", keyword_to_string(token.type));
    else // symbol
      printf("symbol: %s\n", symbol_to_string(token.type));
  }
}

/*
 * Consumption
 */

static int token_next_pos = 0;

token_t *token_consume(void)
{
  token_t *tokens_arr = VECTOR_ARR(tokens, token_t);
  if (token_next_pos == tokens->length)
    return NULL;
  return &tokens_arr[token_next_pos++];
}

token_t *token_peek(int n)
{
  if (n <= 0)
    exit_with_error("token_peek(): n must be a positive integer");

  token_t *tokens_arr = VECTOR_ARR(tokens, token_t);
  if (token_next_pos + n - 1 >= tokens->length)
    return NULL;
  return &tokens_arr[token_next_pos + n - 1];
}

static token_t *token_assertn(int n, token_t *token, ...)
{
  va_list types;
  if (token == NULL)
    exit_parsing_error_no_tokens();

  for (va_start(types, token); n--;)
  {
    if (token->type == va_arg(types, token_type_t))
    {
      va_end(types);
      return token;
    }
  }

  va_end(types);
  exit_parsing_error_unexpected_token(token);
}

token_t *token_assert(token_t *token, token_type_t t1)
{
  return token_assertn(1, token, t1);
}

token_t *token_assert2(token_t *token, token_type_t t1, token_type_t t2)
{
  return token_assertn(2, token, t1, t2);
}

token_t *token_assert3(token_t *token, token_type_t t1, token_type_t t2,
                      token_type_t t3)
{
  return token_assertn(3, token, t1, t2, t3);
}

token_t *token_assert4(token_t *token, token_type_t t1, token_type_t t2,
                      token_type_t t3, token_type_t t4)
{
  return token_assertn(4, token, t1, t2, t3, t4);
}

static token_t *token_checkn(int n, token_t *token, ...)
{
  va_list types;
  if (token == NULL)
    return NULL;

  for (va_start(types, token); n--;)
  {
    if (token->type == va_arg(types, token_type_t))
    {
      va_end(types);
      return token;
    }
  }

  va_end(types);
  return NULL;
}

token_t *token_check(token_t *token, token_type_t t1)
{
  return token_checkn(1, token, t1);
}

token_t *token_check2(token_t *token, token_type_t t1, token_type_t t2)
{
  return token_checkn(2, token, t1, t2);
}

token_t *token_check3(token_t *token, token_type_t t1, token_type_t t2,
                      token_type_t t3)
{
  return token_checkn(3, token, t1, t2, t3);
}

token_t *token_check4(token_t *token, token_type_t t1, token_type_t t2,
                      token_type_t t3, token_type_t t4)
{
  return token_checkn(4, token, t1, t2, t3, t4);
}
