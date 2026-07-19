#include <stdio.h>
#include <string.h>
#include <stdlib.h>
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

void print_with_level(char *str, int level)
{
  for (int i = 0; i < level; i++)
    printf("  ");
  printf("- %s\n", str);
}

#define CASE_UNARY(type, name) \
  case type: { \
    print_with_level(name, level); \
    ast_node_binary_expr_t *data = node->data; \
    print_ast(data->left_operand, level + 1); \
    break; \
  }

#define CASE_BINARY(type, name) \
  case type: { \
    print_with_level(name, level); \
    ast_node_binary_expr_t *data = node->data; \
    print_ast(data->left_operand, level + 1); \
    print_ast(data->right_operand, level + 1); \
    break; \
  }

void print_ast(ast_node_t *node, int level)
{
  switch (node->type)
  {
    case AST_EXPR_PRIMITIVE: {
      ast_node_primitive_expr_t *data = node->data;
      char buf[100];
      switch (data->token->type)
      {
        case TOKEN_IDENTIFIER:
          sprintf(buf, "Identifier %s", data->token->identifier);
          break;
        case TOKEN_CONSTANT:
          sprintf(buf, "Constant %d", data->token->constant);
          break;
        default:
          break;
      }
      print_with_level(buf, level);
      break;
    }

    case AST_EXPR_FUNCTION_CALL: {
      ast_node_function_call_expr_t *data = node->data;
      print_with_level("Function call", level);
      print_with_level("Callee", level + 1);
      print_ast(data->callee, level + 2);
      print_with_level("Arguments", level + 1);
      for (int i = 0; i < data->arguments_size; i++)
        print_ast(data->arguments[i], level + 2);
      break;
    }

      CASE_UNARY(AST_EXPR_LOGICAL_NEGATION, "Logical negation");

      CASE_BINARY(AST_EXPR_ADDITION, "Addition");
      CASE_BINARY(AST_EXPR_SUBTRACTION, "Subtraction");
      CASE_BINARY(AST_EXPR_MULTIPLICATION, "Multiplication");
      CASE_BINARY(AST_EXPR_DIVISION, "Division");
      CASE_BINARY(AST_EXPR_MODULO, "Modulo");
      CASE_BINARY(AST_EXPR_LESS_THAN, "<");
      CASE_BINARY(AST_EXPR_GREATER_THAN, ">");
      CASE_BINARY(AST_EXPR_LESS_THAN_EQUAL_TO, "<=");
      CASE_BINARY(AST_EXPR_GREATER_THAN_EQUAL_TO, ">=");
      CASE_BINARY(AST_EXPR_EQUALS, "==");
      CASE_BINARY(AST_EXPR_NOT_EQUALS, "!=");
      CASE_BINARY(AST_EXPR_LOGICAL_AND, "&&");
      CASE_BINARY(AST_EXPR_LOGICAL_OR, "||");
      CASE_BINARY(AST_EXPR_ASSIGNMENT, "=");

    case AST_STMT_LIST: {
      ast_node_stmt_list_t *data = node->data;
      print_with_level("Statement list", level);
      for (int i = 0; i < data->stmts_size; i++)
        print_ast(data->stmts[i], level + 1);
      break;
    }

    case AST_STMT_EXPR: {
      print_with_level("Expression statement", level);
      print_ast(DATA_STMT(node)->expr_0, level + 1);
      break;
    }

    case AST_STMT_IF: {
      print_with_level("If statement", level);
      print_ast(DATA_STMT(node)->expr_0, level + 1);
      print_ast(DATA_STMT(node)->stmt_list, level + 1);
      break;
    }

    case AST_STMT_WHILE: {
      print_with_level("While statement", level);
      print_ast(DATA_STMT(node)->expr_0, level + 1);
      print_ast(DATA_STMT(node)->stmt_list, level + 1);
      break;
    }

    case AST_STMT_RETURN: {
      print_with_level("Return statement", level);
      print_ast(DATA_STMT(node)->expr_0, level + 1);
      break;
    }

    default: {
      exit_with_error("Not supported\n");
      break;
    }
  }
}

int main(void)
{
  printf("TOKENIZING\n");

  tokenize();

  for (int i = 0; i < tokens_size; i++)
  {
    if (tokens[i].type == TOKEN_IDENTIFIER)
      printf("identifier: %s\n", tokens[i].identifier);
    else if (tokens[i].type == TOKEN_CONSTANT)
      printf("constant: %d\n", tokens[i].constant);
    else if (IS_KEYWORD(tokens[i]))
      printf("keyword: %s\n", keyword_to_string(tokens[i].type));
    else // symbol
      printf("symbol: %s\n", symbol_to_string(tokens[i].type));
  }

  printf("PARSING\n");

  ast_node_t *root = parse();
  print_ast(root, 0);

  return 0;
}
