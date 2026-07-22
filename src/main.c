#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "yiche.h"

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

int main(void)
{
  input_init();

  printf("TOKENIZING\n");

  tokenize();

  token_t *tokens_arr = VECTOR_ARR(tokens, token_t);

  for (int i = 0; i < tokens->length; i++)
  {
    token_t token = tokens_arr[i];
    if (token.type == TOKEN_IDENTIFIER)
      printf("identifier: %s\n", token.identifier);
    else if (token.type == TOKEN_CONSTANT)
      printf("constant: %d\n", token.constant);
    else if (IS_KEYWORD(token))
      printf("keyword: %s\n", keyword_to_string(token.type));
    else // symbol
      printf("symbol: %s\n", symbol_to_string(token.type));
  }

  printf("PARSING\n");

  ast_node_t *root = parse();
  ast_node_print(root);

  return 0;
}
