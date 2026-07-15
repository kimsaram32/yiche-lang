#include <stdio.h>
#include "yiche.h"

static char *keyword_to_string(token_type_t keyword_type)
{
  switch (keyword_type)
  {
    case TOKEN_KEYWORD_FLOAT:
      return "float";
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

int main(void)
{
  tokenize();

  for (int i = 0; i < tokens_size; i++)
    switch (tokens[i].type)
    {
      case TOKEN_IDENTIFIER:
        printf("identifier: %s\n", tokens[i].identifier);
        break;
      case TOKEN_SYMBOL:
        printf("symbol: %s\n", tokens[i].symbol);
        break;
      case TOKEN_CONSTANT:
        printf("constant: %d\n", tokens[i].constant);
        break;
      default:
        printf("keyword: %s\n", keyword_to_string(tokens[i].type));
        break;
    }
}
