#include <stdlib.h>
#include "yiche.h"

static void _ast_node_print(ast_node_t *node, int level);

void ast_node_print(ast_node_t *node)
{
  _ast_node_print(node, 0);
}

static void print_indent(int level)
{
  for (int i = 0; i < level; i++)
    printf("  ");
}

static char *data_type_to_string(data_type_t data_type)
{
  switch (data_type)
  {
    case DATA_TYPE_INT:
      return "int";
  }
}

#define AST_NODE_DEFINE_PRINT_FUNCTION(name) \
  static void ast_node_ ## name ## _print(ast_node_t *node, int level) \
  { \
    ast_node_ ## name ## _t *data = node->data; \
    printf("%s {\n", # name);

#define AST_NODE_DEFINE_PRINT_FUNCTION_END \
    print_indent(level); \
    printf("}\n"); \
  }

#define AST_NODE_PRINT_ATTR(label, fmt, ...) do { \
    print_indent(level + 1); \
    printf("%s = ", label); \
    printf(fmt "\n", __VA_ARGS__); \
  } \
  while (0)

#define AST_NODE_PRINT_CHILDREN(label, children_vector) do { \
    print_indent(level + 1); \
    printf("%s[%d] = {\n", label, children_vector->length); \
    for (int i = 0; i < children_vector->length; i++) \
    { \
      print_indent(level + 2); \
      _ast_node_print(VECTOR_ARR(children_vector, ast_node_t*)[i], level + 2); \
    } \
    print_indent(level + 1); \
    printf("}\n"); \
  } \
  while (0)

#define AST_NODE_PRINT_CHILD(label, child) do { \
    print_indent(level + 1); \
    printf("%s = ", label); \
    _ast_node_print(child, level + 1); \
  } \
  while (0)

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

AST_NODE_DEFINE_PRINT_FUNCTION(primitive_expr)
{
  switch (data->token->type)
  {
    case TOKEN_IDENTIFIER:
      AST_NODE_PRINT_ATTR("identifier", "%s", data->token->identifier);
      break;
    case TOKEN_CONSTANT:
      AST_NODE_PRINT_ATTR("constant", "%d", data->token->constant);
      break;
    default:
      __builtin_unreachable();
  }
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

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

AST_NODE_DEFINE_PRINT_FUNCTION(unary_expr)
{
  char *operator;
  switch (node->type)
  {
    case AST_EXPR_LOGICAL_NEGATION:
      operator = "!";
      break;
    default:
      __builtin_unreachable();
  }
  AST_NODE_PRINT_ATTR("operator", "%s", operator);
  AST_NODE_PRINT_CHILD("operand", data->operand);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

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

AST_NODE_DEFINE_PRINT_FUNCTION(binary_expr)
{
  char *operator;
  switch (node->type)
  {
    case AST_EXPR_ADDITION:
      operator = "+";
      break;
    case AST_EXPR_SUBTRACTION:
      operator = "-";
      break;
    case AST_EXPR_MULTIPLICATION:
      operator = "*";
      break;
    case AST_EXPR_DIVISION:
      operator = "/";
      break;
    case AST_EXPR_MODULO:
      operator = "%";
      break;
    case AST_EXPR_LESS_THAN:
      operator = "<";
      break;
    case AST_EXPR_GREATER_THAN:
      operator = ">";
      break;
    case AST_EXPR_LESS_THAN_EQUAL_TO:
      operator = "<=";
      break;
    case AST_EXPR_GREATER_THAN_EQUAL_TO:
      operator = ">=";
      break;
    case AST_EXPR_EQUALS:
      operator = "==";
      break;
    case AST_EXPR_NOT_EQUALS:
      operator = "!=";
      break;
    case AST_EXPR_LOGICAL_AND:
      operator = "&&";
      break;
    case AST_EXPR_LOGICAL_OR:
      operator = "||";
      break;
    case AST_EXPR_ASSIGNMENT:
      operator = "=";
      break;
    default:
      __builtin_unreachable();
  }
  AST_NODE_PRINT_ATTR("operator", "%s", operator);
  AST_NODE_PRINT_CHILD("left", data->left_operand);
  AST_NODE_PRINT_CHILD("right", data->right_operand);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

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

AST_NODE_DEFINE_PRINT_FUNCTION(function_call_expr)
{
  AST_NODE_PRINT_CHILD("callee", data->callee);
  AST_NODE_PRINT_CHILDREN("arguments", data->arguments);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

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

AST_NODE_DEFINE_PRINT_FUNCTION(stmt_list)
{
  AST_NODE_PRINT_CHILDREN("stmts", data->stmts);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

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

AST_NODE_DEFINE_PRINT_FUNCTION(stmt)
{
  char *type;
  switch (node->type)
  {
    case AST_STMT_EXPR:
      type = "expr";
      break;
    case AST_STMT_IF:
      type = "if";
      break;
    case AST_STMT_WHILE:
      type = "while";
      break;
    case AST_STMT_RETURN:
      type = "return";
      break;
    default:
      __builtin_unreachable();
  }
  AST_NODE_PRINT_ATTR("type", "%s", type);
  AST_NODE_PRINT_CHILD("expr", data->expr_0);

  if (node->type == AST_STMT_IF || node->type == AST_STMT_WHILE)
    AST_NODE_PRINT_CHILD("stmt_list", data->stmt_list);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

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

AST_NODE_DEFINE_PRINT_FUNCTION(variable_decl)
{
  AST_NODE_PRINT_ATTR("identifier", "%s", data->token_identifier->identifier);
  AST_NODE_PRINT_ATTR("data_type", "%s", data_type_to_string(data->data_type));

  if (data->initializer != NULL)
    AST_NODE_PRINT_CHILD("initializer", data->initializer);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

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

AST_NODE_DEFINE_PRINT_FUNCTION(function_decl)
{
  AST_NODE_PRINT_ATTR("identifier", "%s", data->token_identifier->identifier);
  AST_NODE_PRINT_ATTR("return_data_type", "%s", data_type_to_string(data->return_data_type));
  AST_NODE_PRINT_CHILDREN("parameters", data->parameters);
  AST_NODE_PRINT_CHILD("body", data->body);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

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

AST_NODE_DEFINE_PRINT_FUNCTION(program)
{
  AST_NODE_PRINT_CHILDREN("decls", data->decls);
}
AST_NODE_DEFINE_PRINT_FUNCTION_END

/*
 * Printing
 */

void _ast_node_print(ast_node_t *node, int level)
{
  switch (node->type)
  {
    // ast_node_program_t
    case AST_PROGRAM:
      ast_node_program_print(node, level);
      break;

    // ast_node_variable_decl_t
    case AST_DECL_VARIABLE:
      ast_node_variable_decl_print(node, level);
      break;

    // ast_node_function_decl_t
    case AST_DECL_FUNCTION:
      ast_node_function_decl_print(node, level);
      break;

    // ast_node_stmt_t
    case AST_STMT_EXPR:
    case AST_STMT_IF:
    case AST_STMT_WHILE:
    case AST_STMT_RETURN:
      ast_node_stmt_print(node, level);
      break;

    // ast_node_stmt_list_t
    case AST_STMT_LIST:
      ast_node_stmt_list_print(node, level);
      break;

    // ast_node_primitive_expr_t
    case AST_EXPR_PRIMITIVE:
      ast_node_primitive_expr_print(node, level);
      break;

    // ast_node_function_call_expr_t
    case AST_EXPR_FUNCTION_CALL:
      ast_node_function_call_expr_print(node, level);
      break;

    // ast_node_unary_expr_t
    case AST_EXPR_LOGICAL_NEGATION:
      ast_node_unary_expr_print(node, level);
      break;

    // ast_node_binary_expr_t
    case AST_EXPR_ADDITION:
    case AST_EXPR_SUBTRACTION:
    case AST_EXPR_MULTIPLICATION:
    case AST_EXPR_DIVISION:
    case AST_EXPR_MODULO:
    case AST_EXPR_LESS_THAN:
    case AST_EXPR_GREATER_THAN:
    case AST_EXPR_LESS_THAN_EQUAL_TO:
    case AST_EXPR_GREATER_THAN_EQUAL_TO:
    case AST_EXPR_EQUALS:
    case AST_EXPR_NOT_EQUALS:
    case AST_EXPR_LOGICAL_AND:
    case AST_EXPR_LOGICAL_OR:
    case AST_EXPR_ASSIGNMENT:
      ast_node_binary_expr_print(node, level);
      break;
  }
}
