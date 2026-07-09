#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_SYMBOL_LENGTH 3

char *valid_symbols[] = {
  ".", ",", ":", ";", "->",
  "+", "-", "*", "/", "%", "++", "--",
  "~", "&", "|", "^", "<<", ">>",
  "<", ">", "<=", ">=", "==", "!=",
  "&&", "||", "!",
  "=", "+=", "-=", "*=", "/=", "%=",
  "~=", "&=", "|=", "^=", "<<=", ">>=",
  "[", "]", "(", ")",
};

int is_valid_symbol(char *s)
{
  for (int i = 0; i < sizeof(valid_symbols) / sizeof(valid_symbols[0]); i++)
    if (!strcmp(valid_symbols[i], s))
      return 1;

  return 0;
}

/*
  ctype.h에 isspace가 있긴 하지만, whitespace character를
  explicit 하게 정하고 싶었습니다.
*/

int is_space(char c)
{
  return c == ' ' || c == '\n' || c == '\r' || c == '\t';
}

typedef char token_buffer_array_t[MAX_SYMBOL_LENGTH + 1];

token_buffer_array_t *tokens = NULL;
int tokens_size = 0;
int tokens_capacity = 64;

/* returns a (MAX_SYMBOL_LENGTH + 1)-byte zero-filled buffer. */
char *next_token_buffer(void)
{
  tokens_size++;

  if (tokens == NULL)
  {
    tokens = malloc(sizeof(tokens[0]) * tokens_capacity);
    if (tokens == NULL)
      exit(EXIT_FAILURE);
    memset(tokens, 0, sizeof(tokens[0]) * tokens_capacity);
  }
  else if (tokens_size > tokens_capacity)
  {
    int new_capacity = tokens_capacity * 2;

    token_buffer_array_t *old = tokens;
    if ((tokens = realloc(tokens, sizeof(tokens[0]) * new_capacity)) == NULL)
    {
      free(old);
      exit(EXIT_FAILURE);
    }

    memset(tokens + tokens_capacity, 0, sizeof(tokens[0]) * (new_capacity - tokens_capacity));

    tokens_capacity = new_capacity;
  }

  return tokens[tokens_size - 1];
}

/*
  limitations: invalid operator 에서 바로 discard 하도록 동작합니다. 만약 그
  자체로는 invalid 하지만 valid operator의 prefix로 가능한 경우가 있다면 제대로
  동작하지 않는 문제가 있습니다. 다만 그런 경우를 찾을 수 없어서 이렇게
  구현하였습니다.

  e.g. +는 그 자체로도 valid 하고, prefix (+= 등) 으로도 valid 합니다. 만약 +
  자체는 invalid 하였다면 제대로 동작하지 않습니다.
*/

void try_single_symbol(char *buf, char c)
{
  *buf = c;

  if (!is_valid_symbol(buf))
  {
    *buf = '\0';
    fprintf(stderr, "invalid symbol %c\n", c);
    exit(EXIT_FAILURE);
  }
}

void tokenize_symbols(FILE *stream)
{
  int c, i = 0;
  char *buf = next_token_buffer();

  while ((c = fgetc(stream)) != EOF)
  {
    if (is_space(c))
    {
      if (i > 0)
        buf = next_token_buffer(), i = 0;
    }
    else if (i == 0)
    {
      try_single_symbol(buf, c);
      i++;
    }
    else
    {
      buf[i] = c;

      if (!is_valid_symbol(buf))
      {
        buf[i] = '\0';
        buf = next_token_buffer(), i = 0;
        try_single_symbol(buf, c);
      }

      if (++i == MAX_SYMBOL_LENGTH)
        buf = next_token_buffer(), i = 0;
    }
  }

  if (!buf[0])
    tokens_size--;
}

int main(void)
{
  tokenize_symbols(stdin);

  for (int i = 0; i < tokens_size; i++)
    printf("%dth token: %s\n", i, tokens[i]);

  return 0;
}
