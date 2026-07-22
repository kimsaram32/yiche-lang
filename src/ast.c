#include <stdlib.h>
#include "yiche.h"

static ast_node_t *ast_node_create(ast_node_type_t type)
{
  ast_node_t *node = malloc(sizeof(ast_node_t));
  if (node == NULL)
    exit_out_of_memory();

  node->type = type;
  node->data = NULL;

  return node;
}

#define AST_NODE_CREATE_WITH_DATA(node, data, type, data_type) do { \
    node = ast_node_create(type); \
    data = malloc(sizeof(data_type)); \
    if (data == NULL) {\
      exit_out_of_memory(); \
    } \
    node->data = data; \
  } \
  while (0)

/*
 * primitive_expr
 */

ast_node_t *ast_node_primitive_expr_create(token_t *token)
{
  ast_node_t *node;
  ast_node_primitive_expr_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, AST_EXPR_PRIMITIVE, ast_node_primitive_expr_t);

  data->token = token;

  return node;
}

/*
 * unary_expr
 */

ast_node_t *ast_node_unary_expr_create(ast_node_type_t type, ast_node_t *operand)
{
  ast_node_t *node;
  ast_node_unary_expr_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, type, ast_node_unary_expr_t);

  data->operand = operand;

  return node;
}

/*
 * binary_expr
 */

ast_node_t *ast_node_binary_expr_create(ast_node_type_t type, ast_node_t *left_operand,
                                               ast_node_t *right_operand)
{
  ast_node_t *node;
  ast_node_binary_expr_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, type, ast_node_binary_expr_t);

  data->left_operand = left_operand;
  data->right_operand = right_operand;

  return node;
}

/*
 * function_call_expr
 */

ast_node_t *ast_node_function_call_expr_create(ast_node_t *callee)
{
  ast_node_t *node;
  ast_node_function_call_expr_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, AST_EXPR_FUNCTION_CALL, ast_node_function_call_expr_t);

  data->callee = callee;
  data->arguments = vector_pointer_create(8);
  if (data->arguments == NULL)
    exit_out_of_memory();

  return node;
}

void ast_node_function_call_expr_append_argument(ast_node_t *node, ast_node_t *arg)
{
  if (!vector_pointer_append(DATA_FUNCTION_CALL_EXPR(node)->arguments, arg))
    exit_out_of_memory();
}

/*
 * stmt_list
 */

ast_node_t *ast_node_stmt_list_create(void)
{
  ast_node_t *node;
  ast_node_stmt_list_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, AST_STMT_LIST, ast_node_stmt_list_t);

  data->stmts = vector_pointer_create(8);
  if (data->stmts == NULL)
    exit_out_of_memory();

  return node;
}

void ast_node_stmt_list_append_stmt(ast_node_t *node, ast_node_t *stmt)
{
  if (!vector_pointer_append(DATA_STMT_LIST(node)->stmts, stmt))
    exit_out_of_memory();
}

/*
 * stmt
 */

ast_node_t *ast_node_stmt_create(ast_node_type_t type, ast_node_t *stmt_list,
                                        ast_node_t *expr_0)
{
  ast_node_t *node;
  ast_node_stmt_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, type, ast_node_stmt_t);

  data->stmt_list = stmt_list;
  data->expr_0 = expr_0;

  return node;
}

/*
 * variable_decl
 */

ast_node_t *ast_node_variable_decl_create(token_t *token_identifier,
                                                 data_type_t data_type, ast_node_t *initializer)
{
  ast_node_t *node;
  ast_node_variable_decl_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, AST_DECL_VARIABLE, ast_node_variable_decl_t);

  data->token_identifier = token_identifier;
  data->data_type = data_type;
  data->initializer = initializer;

  return node;
}

/*
 * function_decl
 */

ast_node_t *ast_node_function_decl_create(token_t *token_identifier)
{
  ast_node_t *node;
  ast_node_function_decl_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, AST_DECL_FUNCTION, ast_node_function_decl_t);

  data->token_identifier = token_identifier;
  data->parameters = vector_pointer_create(8);
  if (data->parameters == NULL)
    exit_out_of_memory();

  return node;
}

void ast_node_function_decl_append_parameter(ast_node_t *node, ast_node_t *parameter)
{
  if (!vector_pointer_append(DATA_FUNCTION_DECL(node)->parameters, parameter))
    exit_out_of_memory();
}

/*
 * program
 */

ast_node_t *ast_node_program_create(void)
{
  ast_node_t *node;
  ast_node_program_t *data;
  AST_NODE_CREATE_WITH_DATA(node, data, AST_PROGRAM, ast_node_program_t);

  data->decls = vector_pointer_create(8);
  if (data->decls == NULL)
    exit_out_of_memory();

  return node;
}

void ast_node_program_append_decl(ast_node_t *node, ast_node_t *decl)
{
  if (!vector_pointer_append(DATA_PROGRAM(node)->decls, decl))
    exit_out_of_memory();
}
