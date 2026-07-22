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

static char *data_type_to_string(data_type_t data_type)
{
  switch (data_type)
  {
    case DATA_TYPE_INT:
      return "int";
  }
}

#define PRINT_WITH_LEVEL(level, ...) \
  do \
  { \
    for (int i = 0; i < level; i++) \
      printf("  "); \
    printf("- "); \
    printf(__VA_ARGS__); \
    printf("\n"); \
  } \
  while (0)

#define CASE_UNARY(type, name) \
  case type: { \
    PRINT_WITH_LEVEL(level, name); \
    ast_node_binary_expr_t *data = node->data; \
    print_ast(data->left_operand, level + 1); \
    break; \
  }

#define CASE_BINARY(type, name) \
  case type: { \
    PRINT_WITH_LEVEL(level, name); \
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
      switch (data->token->type)
      {
        case TOKEN_IDENTIFIER:
          PRINT_WITH_LEVEL(level, "Identifier %s", data->token->identifier);
          break;
        case TOKEN_CONSTANT:
          PRINT_WITH_LEVEL(level, "Constant %d", data->token->constant);
          break;
        default:
          break;
      }
      break;
    }

    case AST_EXPR_FUNCTION_CALL: {
      ast_node_function_call_expr_t *data = node->data;
      PRINT_WITH_LEVEL(level, "Function call");
      PRINT_WITH_LEVEL(level + 1, "Callee");
      print_ast(data->callee, level + 2);
      PRINT_WITH_LEVEL(level + 1, "Arguments");
      for (int i = 0; i < data->arguments->length; i++)
        print_ast(VECTOR_ARR(data->arguments, ast_node_t*)[i], level + 2);
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
      PRINT_WITH_LEVEL(level, "Statement list");
      for (int i = 0; i < data->stmts->length; i++)
        print_ast(VECTOR_ARR(data->stmts, ast_node_t*)[i], level + 1);
      break;
    }

    case AST_STMT_EXPR: {
      PRINT_WITH_LEVEL(level, "Expression statement");
      print_ast(DATA_STMT(node)->expr_0, level + 1);
      break;
    }

    case AST_STMT_IF: {
      PRINT_WITH_LEVEL(level, "If statement");
      print_ast(DATA_STMT(node)->expr_0, level + 1);
      print_ast(DATA_STMT(node)->stmt_list, level + 1);
      break;
    }

    case AST_STMT_WHILE: {
      PRINT_WITH_LEVEL(level, "While statement");
      print_ast(DATA_STMT(node)->expr_0, level + 1);
      print_ast(DATA_STMT(node)->stmt_list, level + 1);
      break;
    }

    case AST_STMT_RETURN: {
      PRINT_WITH_LEVEL(level, "Return statement");
      print_ast(DATA_STMT(node)->expr_0, level + 1);
      break;
    }

    case AST_DECL_VARIABLE: {
      ast_node_variable_decl_t *data = node->data;
      PRINT_WITH_LEVEL(level, "Variable declaration");
      PRINT_WITH_LEVEL(level, "Identifier: %s", data->token_identifier->identifier);
      PRINT_WITH_LEVEL(level, "Return type: %s", data_type_to_string(data->data_type));

      if (data->initializer != NULL)
      {
        PRINT_WITH_LEVEL(level + 1, "Initializer");
        print_ast(data->initializer, level + 2);
      }

      break;
    }

    case AST_DECL_FUNCTION: {
      ast_node_function_decl_t *data = node->data;
      PRINT_WITH_LEVEL(level, "Function declaration");
      PRINT_WITH_LEVEL(level, "Identifier: %s", data->token_identifier->identifier);
      PRINT_WITH_LEVEL(level, "Return type: %s", data_type_to_string(data->return_data_type));

      PRINT_WITH_LEVEL(level + 1, "Parameters");
      for (int i = 0; i < data->parameters->length; i++)
        print_ast(VECTOR_ARR(data->parameters, ast_node_t*)[i], level + 2);
      print_ast(data->body, level + 1);

      break;
    }

    case AST_PROGRAM: {
      ast_node_program_t *data = node->data;
      PRINT_WITH_LEVEL(level, "Program");

      for (int i = 0; i < data->decls->length; i++)
        print_ast(VECTOR_ARR(data->decls, ast_node_t*)[i], level + 1);

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
  print_ast(root, 0);

  return 0;
}
