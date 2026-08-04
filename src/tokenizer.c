#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "yiche.h"

typedef struct tokenizer_context_t
{
  VECTOR_T(token_t) *tokens;
  input_context_t *input_ctx;
  int next_read_pos;
}
tokenizer_context_t;

static void vector_token_free(void *p)
{
  token_t *token = p;

  if (token->identifier != NULL)
    free(token->identifier);
  free(token->lexeme);
}

tokenizer_context_t *tokenizer_context_create(FILE *stream)
{
  tokenizer_context_t *ctx = malloc(sizeof(tokenizer_context_t));
  if (ctx == NULL)
    return NULL;

  if ((ctx->tokens = vector_create(sizeof(token_t), 64, vector_token_free)) == NULL)
  {
    free(ctx);
    return NULL;
  }

  if ((ctx->input_ctx = input_context_create(stream)) == NULL)
  {
    vector_free(ctx->tokens);
    free(ctx);
    return NULL;
  }

  ctx->next_read_pos = 0;

  return ctx;
}

void tokenizer_context_free(tokenizer_context_t *ctx)
{
  input_context_free(ctx->input_ctx);
  vector_free(ctx->tokens);
  free(ctx);
}

void tokenizer_print_error(tokenizer_context_t *ctx, tokenizer_result_t result)
{
  if (result == TOKENIZER_SUCCESS)
    return;

  if (result == TOKENIZER_OOM)
  {
    fprintf(stdout, "out of memory");
    return;
  }

  char *s;
  switch (result)
  {
    case TOKENIZER_COMMENT_MULTI_UNCLOSED:
      s = "unclosed multi-line comment";
      break;
    case TOKENIZER_SYMBOL_INVALID:
      s = "invalid symbol";
      break;
    case TOKENIZER_CHARACTER_CONSTANT_EMPTY:
      s = "empty character constant '' not allowed";
      break;
    case TOKENIZER_CHARACTER_CONSTANT_INVALID_ESCAPE:
      s = "invalid escape sequence";
      break;
    case TOKENIZER_CHARACTER_CONSTANT_INVALID:
      s = "invalid character constant sequence";
      break;
    default:
      UNREACHABLE;
  }

  input_char_t last_char;
  if (input_get_last_char(ctx->input_ctx, &last_char) != INPUT_SUCCESS)
    return;

  fprintf(stdout, "lexical error at line %d, column %d ('%c'):\n%s",
          last_char.line, last_char.column, last_char.c, s);
}

/*
 * Helpers
 */

#define PEEK(ctx, c, n) do \
  { \
    if (input_peek_char((ctx)->input_ctx, (c), (n)) != INPUT_SUCCESS) \
      return TOKENIZER_OOM; \
  } \
  while (0)

#define CONSUME(ctx, c) do \
  { \
    if (input_consume_char((ctx)->input_ctx, (c)) != INPUT_SUCCESS) \
      return TOKENIZER_OOM; \
  } \
  while (0)

/*
 * Tokenizing
 */

static tokenizer_result_t skip_multi_line_comment(tokenizer_context_t *ctx)
{
  char c;
  int can_exit = 0;

  while (1)
  {
    CONSUME(ctx, &c);

    if (c == 0)
      break;

    if (c == '*')
      can_exit = 1;
    else if (c == '/' && can_exit)
      return TOKENIZER_SUCCESS;
  }

  return TOKENIZER_COMMENT_MULTI_UNCLOSED;
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
static tokenizer_result_t read_keyword_or_identifier(tokenizer_context_t *ctx, char **buf_p, char initial)
{
  int buf_capacity = 8, i = 1;
  char *buf;

  if ((buf = malloc(sizeof(char) * buf_capacity)) == NULL)
    return TOKENIZER_OOM;

  buf[0] = initial;

  while (1)
  {
    if (input_peek_char(ctx->input_ctx, buf + i, 1) != INPUT_SUCCESS)
    {
      free(buf);
      return TOKENIZER_OOM;
    }

    if (!is_letter(buf[i]) && !is_digit(buf[i]))
      break;

    if (input_consume_char(ctx->input_ctx, NULL) != INPUT_SUCCESS)
    {
      free(buf);
      return TOKENIZER_OOM;
    }

    if (++i == buf_capacity)
    {
      buf_capacity *= 2;
      char *old = buf;
      if ((buf = realloc(buf, sizeof(char) * buf_capacity)) == NULL)
      {
        free(old);
        return TOKENIZER_OOM;
      }
    }
  }

  buf[i] = '\0';

  *buf_p = buf;
  return TOKENIZER_SUCCESS;
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

// On success, '*type_p' is set to a symbol type.
static tokenizer_result_t read_symbol(tokenizer_context_t *ctx, token_type_t *type_p, char c1)
{
  char c2;
  int type;

  PEEK(ctx, &c2, 1);

  if (c2 != 0)
  {
    type = get_symbol_2(c1, c2);
    if (type != -1)
    {
      CONSUME(ctx, NULL);

      *type_p = type;
      return TOKENIZER_SUCCESS;
    }
  }

  if ((type = get_symbol_1(c1)) == -1)
    return TOKENIZER_SYMBOL_INVALID;

  *type_p = type;
  return TOKENIZER_SUCCESS;
}

static int is_hexadecimal_digit(char c)
{
  return is_digit(c) || (c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f');
}

static tokenizer_result_t read_numeric_constant(tokenizer_context_t *ctx, int *val, char initial)
{
  char c, c2;
  *val = 0;

  if (initial >= '1' && initial <= '9')
  {
    // decimal constant greater than 0
    *val = *val * 10 + initial - '0';

    while (1)
    {
      PEEK(ctx, &c, 1);

      if (!is_digit(c))
        break;

      CONSUME(ctx, NULL);

      *val = *val * 10 + c - '0';
    }
  }
  else
  {
    PEEK(ctx, &c, 1);
    PEEK(ctx, &c2, 2);

    if ((c == 'x' || c == 'X') && is_hexadecimal_digit(c2))
    {
      // hexadecimal constant
      CONSUME(ctx, NULL);

      while (1)
      {
        PEEK(ctx, &c, 1);

        if (!is_hexadecimal_digit(c))
          break;

        CONSUME(ctx, NULL);

        *val *= 16;
        if (c >= 'a')
          *val += c - 'a' + 10;
        else if (c >= 'A')
          *val += c - 'A' + 10;
        else
          *val += c - '0';
      }
    } // else: decimal constant zero
  }

  return TOKENIZER_SUCCESS;
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

static tokenizer_result_t read_character_constant(tokenizer_context_t *ctx, int *val)
{
  char c;

  CONSUME(ctx, &c);

  if (c == '\'')
    return TOKENIZER_CHARACTER_CONSTANT_EMPTY;
  else if (c == '\\')
  {
    CONSUME(ctx, &c);

    *val = get_escape_character_value(c);
    if (*val == -1)
      return TOKENIZER_CHARACTER_CONSTANT_INVALID_ESCAPE;
  }
  else if (is_visible_character(c) || c == ' ' || c == '\t')
    *val = c;
  else
    return TOKENIZER_CHARACTER_CONSTANT_INVALID;

  CONSUME(ctx, &c);

  if (c != '\'')
    return TOKENIZER_CHARACTER_CONSTANT_INVALID;

  return TOKENIZER_SUCCESS;
}

tokenizer_result_t tokenizer_process(tokenizer_context_t *ctx)
{
  tokenizer_result_t result;
  char c, c2;

  while (1)
  {
    CONSUME(ctx, &c);

    if (c == 0)
      break;

    if (is_whitespace(c))
      goto skip;

    if (c == '/')
    {
      PEEK(ctx, &c2, 1);

      if (c2 == '/')
      {
        // technically '\n' is not part of <single_line_comment>, but it'll get
        // skipped anyway, so do not input_unget_char(ctx->input_ctx) it
        do
          CONSUME(ctx, &c);
        while (c != 0 && c != '\n');

        goto skip;
      }
      else if (c2 == '*')
      {
        CONSUME(ctx, NULL);
        if ((result = skip_multi_line_comment(ctx)) != TOKENIZER_SUCCESS)
          return result;
        goto skip;
      }
    }

    token_t token;
    token.identifier = NULL;
    // Never fails
    input_get_last_char(ctx->input_ctx, &token.char_begin);

    if (is_letter(c))
    {
      char *string_or_identifier;
      if ((result = read_keyword_or_identifier(ctx, &string_or_identifier, c))
          != TOKENIZER_SUCCESS)
        return result;

      int keyword_type = string_to_keyword(string_or_identifier);
      if (keyword_type == -1)
      {
        token.identifier = string_or_identifier;
        token.type = TOKEN_IDENTIFIER;
      }
      else
      {
        free(string_or_identifier);
        token.type = keyword_type;
      }
    }
    else if (is_digit(c))
    {
      token.type = TOKEN_CONSTANT;
      if ((result = read_numeric_constant(ctx, &token.constant, c)) != TOKENIZER_SUCCESS)
        return result;
    }
    else if (c == '\'')
    {
      token.type = TOKEN_CONSTANT;
      if ((result = read_character_constant(ctx, &token.constant)) != TOKENIZER_SUCCESS)
        return result;
    }
    else
      if ((result = read_symbol(ctx, &token.type, c)) != TOKENIZER_SUCCESS)
        return result;

    // Never fails
    input_get_last_char(ctx->input_ctx, &token.char_end);

    if ((token.lexeme = input_get_and_clear_buffer(ctx->input_ctx)) == NULL)
    {
      if (token.identifier != NULL)
        free(token.identifier);
      return TOKENIZER_OOM;
    }

    if (!vector_append(ctx->tokens, &token))
    {
      if (token.identifier != NULL)
        free(token.identifier);
      return TOKENIZER_OOM;
    }

    continue;

    skip: free(input_get_and_clear_buffer(ctx->input_ctx));
  }

  return TOKENIZER_SUCCESS;
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

void tokenizer_print_tokens(tokenizer_context_t *ctx)
{
  token_t *tokens_arr = VECTOR_ARR(ctx->tokens, token_t);

  for (int i = 0; i < ctx->tokens->length; i++)
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

token_t *token_consume(tokenizer_context_t *ctx)
{
  token_t *tokens_arr = VECTOR_ARR(ctx->tokens, token_t);
  if (ctx->next_read_pos == ctx->tokens->length)
    return NULL;
  return &tokens_arr[ctx->next_read_pos++];
}

token_t *token_peek(tokenizer_context_t *ctx, int n)
{
  if (n <= 0)
    exit_with_error("token_peek(): n must be a positive integer");

  token_t *tokens_arr = VECTOR_ARR(ctx->tokens, token_t);
  if (ctx->next_read_pos + n - 1 >= ctx->tokens->length)
    return NULL;
  return &tokens_arr[ctx->next_read_pos + n - 1];
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
